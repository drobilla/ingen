/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Controller.hpp"

#include "Buffer.hpp"
#include "BufferFactory.hpp"
#include "BufferRef.hpp"
#include "InputPort.hpp"
#include "InternalPlugin.hpp"
#include "OutputPort.hpp"
#include "PortType.hpp"
#include "RunContext.hpp"

#include <ingen/Atom.hpp>
#include <ingen/Forge.hpp>
#include <ingen/URI.hpp>
#include <ingen/URIs.hpp>
#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/midi/midi.h>
#include <raul/Array.hpp>
#include <raul/Maid.hpp>
#include <raul/Symbol.hpp>

#include <cassert>
#include <cmath>
#include <initializer_list>

namespace ingen::server {

class GraphImpl;

namespace internals {

InternalPlugin* ControllerNode::internal_plugin(URIs& uris) {
	return new InternalPlugin(
		uris, URI(NS_INTERNALS "Controller"), raul::Symbol("controller"));
}

ControllerNode::ControllerNode(InternalPlugin*      plugin,
                               BufferFactory&       bufs,
                               const raul::Symbol&  symbol,
                               bool                 polyphonic,
                               GraphImpl*           parent,
                               SampleRate           srate)
	: InternalBlock(plugin, symbol, false, parent, srate)
{
	const ingen::URIs& uris = bufs.uris();
	_ports = bufs.maid().make_managed<Ports>(7);

	const Atom zero       = bufs.forge().make(0.0f);
	const Atom one        = bufs.forge().make(1.0f);
	const Atom atom_Float = bufs.forge().make_urid(URI(LV2_ATOM__Float));

	_midi_in_port = new InputPort(bufs, this, raul::Symbol("input"), 0, 1,
	                              PortType::ATOM, uris.atom_Sequence, Atom());
	_midi_in_port->set_property(uris.lv2_name, bufs.forge().alloc("Input"));
	_midi_in_port->set_property(uris.atom_supports,
	                            bufs.forge().make_urid(uris.midi_MidiEvent));
	_ports->at(0) = _midi_in_port;

	_midi_out_port = new OutputPort(bufs, this, raul::Symbol("event"), 1, 1,
	                                PortType::ATOM, uris.atom_Sequence, Atom());
	_midi_out_port->set_property(uris.lv2_name, bufs.forge().alloc("Event"));
	_midi_out_port->set_property(uris.atom_supports,
	                             bufs.forge().make_urid(uris.midi_MidiEvent));
	_ports->at(1) = _midi_out_port;

	_param_port = new InputPort(bufs, this, raul::Symbol("controller"), 2, 1,
	                            PortType::ATOM, uris.atom_Sequence, zero);
	_param_port->set_property(uris.atom_supports, atom_Float);
	_param_port->set_property(uris.lv2_minimum, zero);
	_param_port->set_property(uris.lv2_maximum, bufs.forge().make(127.0f));
	_param_port->set_property(uris.lv2_portProperty, uris.lv2_integer);
	_param_port->set_property(uris.lv2_name, bufs.forge().alloc("Controller"));
	_ports->at(2) = _param_port;

	_log_port = new InputPort(bufs, this, raul::Symbol("logarithmic"), 3, 1,
	                          PortType::ATOM, uris.atom_Sequence, zero);
	_log_port->set_property(uris.atom_supports, atom_Float);
	_log_port->set_property(uris.lv2_portProperty, uris.lv2_toggled);
	_log_port->set_property(uris.lv2_name, bufs.forge().alloc("Logarithmic"));
	_ports->at(3) = _log_port;

	_min_port = new InputPort(bufs, this, raul::Symbol("minimum"), 4, 1,
	                          PortType::ATOM, uris.atom_Sequence, zero);
	_min_port->set_property(uris.atom_supports, atom_Float);
	_min_port->set_property(uris.lv2_name, bufs.forge().alloc("Minimum"));
	_ports->at(4) = _min_port;

	_max_port = new InputPort(bufs, this, raul::Symbol("maximum"), 5, 1,
	                          PortType::ATOM, uris.atom_Sequence, one);
	_max_port->set_property(uris.atom_supports, atom_Float);
	_max_port->set_property(uris.lv2_name, bufs.forge().alloc("Maximum"));
	_ports->at(5) = _max_port;

	_audio_port = new OutputPort(bufs, this, raul::Symbol("output"), 6, 1,
	                             PortType::ATOM, uris.atom_Sequence, zero);
	_audio_port->set_property(uris.atom_supports, atom_Float);
	_audio_port->set_property(uris.lv2_name, bufs.forge().alloc("Output"));
	_ports->at(6) = _audio_port;
}

void
ControllerNode::run(RunContext& ctx)
{
	const BufferRef midi_in  = _midi_in_port->buffer(0);
	auto*           seq      = midi_in->get<LV2_Atom_Sequence>();
	const BufferRef midi_out = _midi_out_port->buffer(0);

	LV2_ATOM_SEQUENCE_FOREACH (seq, ev) {
		const auto* buf = static_cast<const uint8_t*>(LV2_ATOM_BODY_CONST(&ev->body));
		if (ev->body.type == _midi_in_port->bufs().uris().midi_MidiEvent &&
		    ev->body.size >= 3 &&
		    lv2_midi_message_type(buf) == LV2_MIDI_MSG_CONTROLLER) {
			if (control(ctx, buf[1], buf[2], ev->time.frames + ctx.start())) {
				midi_out->append_event(ev->time.frames, &ev->body);
			}
		}
	}
}

bool
ControllerNode::control(const RunContext& ctx,
                        const uint8_t     control_num,
                        const uint8_t     val,
                        const FrameTime   time)
{
	assert(time >= ctx.start() && time <= ctx.end());
	const uint32_t offset = time - ctx.start();

	const Sample nval = (val / 127.0f); // normalized [0, 1]

	if (_learning) {
		_param_port->set_control_value(ctx, time, control_num);
		_param_port->force_monitor_update();
		_learning = false;
	} else {
		_param_port->update_values(offset, 0);
	}

	if (control_num != _param_port->buffer(0)->value_at(offset)) {
		return false;
	}

	for (const auto& port : { _min_port, _max_port, _log_port }) {
		port->update_values(offset, 0);
	}

	const Sample min_port_val = _min_port->buffer(0)->value_at(offset);
	const Sample max_port_val = _max_port->buffer(0)->value_at(offset);
	const Sample log_port_val = _log_port->buffer(0)->value_at(offset);

	Sample scaled_value = 0.0f;
	if (log_port_val > 0.0f) {
		// haaaaack, stupid negatives and logarithms
		Sample log_offset = 0.0f;
		if (min_port_val < 0) {
			log_offset = fabsf(min_port_val);
		}
		const Sample min = logf(min_port_val + 1 + log_offset);
		const Sample max = logf(max_port_val + 1 + log_offset);
		scaled_value = expf((nval * (max - min)) + min) - 1 - log_offset;
	} else {
		scaled_value = ((nval) * (max_port_val - min_port_val)) + min_port_val;
	}

	_audio_port->set_control_value(ctx, time, scaled_value);

	return true;
}

} // namespace internals
} // namespace ingen::server
