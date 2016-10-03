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

#include <cmath>

#include "ingen/URIs.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"

#include "Buffer.hpp"
#include "Engine.hpp"
#include "InputPort.hpp"
#include "InternalPlugin.hpp"
#include "OutputPort.hpp"
#include "RunContext.hpp"
#include "ingen_config.h"
#include "internals/Trigger.hpp"
#include "util.hpp"

using namespace std;

namespace Ingen {
namespace Server {
namespace Internals {

InternalPlugin* TriggerNode::internal_plugin(URIs& uris) {
	return new InternalPlugin(
		uris, Raul::URI(NS_INTERNALS "Trigger"), Raul::Symbol("trigger"));
}

TriggerNode::TriggerNode(InternalPlugin*     plugin,
                         BufferFactory&      bufs,
                         const Raul::Symbol& symbol,
                         bool                polyphonic,
                         GraphImpl*          parent,
                         SampleRate          srate)
	: InternalBlock(plugin, symbol, false, parent, srate)
	, _learning(false)
{
	const Ingen::URIs& uris = bufs.uris();
	_ports = new Raul::Array<PortImpl*>(6);

	const Atom zero = bufs.forge().make(0.0f);

	_midi_in_port = new InputPort(bufs, this, Raul::Symbol("input"), 0, 1,
	                              PortType::ATOM, uris.atom_Sequence, Atom());
	_midi_in_port->set_property(uris.lv2_name, bufs.forge().alloc("Input"));
	_midi_in_port->set_property(uris.atom_supports,
	                            bufs.forge().make_urid(uris.midi_MidiEvent));
	_ports->at(0) = _midi_in_port;

	_midi_out_port = new OutputPort(bufs, this, Raul::Symbol("event"), 1, 1,
	                                PortType::ATOM, uris.atom_Sequence, Atom());
	_midi_out_port->set_property(uris.lv2_name, bufs.forge().alloc("Event"));
	_midi_out_port->set_property(uris.atom_supports,
	                             bufs.forge().make_urid(uris.midi_MidiEvent));
	_ports->at(1) = _midi_out_port;

	_note_port = new InputPort(bufs, this, Raul::Symbol("note"), 2, 1,
	                           PortType::ATOM, uris.atom_Sequence,
	                           bufs.forge().make(60.0f));
	_note_port->set_property(uris.atom_supports, bufs.uris().atom_Float);
	_note_port->set_property(uris.lv2_minimum, zero);
	_note_port->set_property(uris.lv2_maximum, bufs.forge().make(127.0f));
	_note_port->set_property(uris.lv2_portProperty, uris.lv2_integer);
	_note_port->set_property(uris.lv2_name, bufs.forge().alloc("Note"));
	_ports->at(2) = _note_port;

	_gate_port = new OutputPort(bufs, this, Raul::Symbol("gate"), 3, 1,
	                            PortType::ATOM, uris.atom_Sequence, zero);
	_gate_port->set_property(uris.atom_supports, bufs.uris().atom_Float);
	_gate_port->set_property(uris.lv2_portProperty, uris.lv2_toggled);
	_gate_port->set_property(uris.lv2_name, bufs.forge().alloc("Gate"));
	_ports->at(3) = _gate_port;

	_trig_port = new OutputPort(bufs, this, Raul::Symbol("trigger"), 4, 1,
	                            PortType::ATOM, uris.atom_Sequence, zero);
	_trig_port->set_property(uris.atom_supports, bufs.uris().atom_Float);
	_trig_port->set_property(uris.lv2_portProperty, uris.lv2_toggled);
	_trig_port->set_property(uris.lv2_name, bufs.forge().alloc("Trigger"));
	_ports->at(4) = _trig_port;

	_vel_port = new OutputPort(bufs, this, Raul::Symbol("velocity"), 5, 1,
	                           PortType::ATOM, uris.atom_Sequence, zero);
	_vel_port->set_property(uris.atom_supports, bufs.uris().atom_Float);
	_vel_port->set_property(uris.lv2_minimum, zero);
	_vel_port->set_property(uris.lv2_maximum, bufs.forge().make(1.0f));
	_vel_port->set_property(uris.lv2_name, bufs.forge().alloc("Velocity"));
	_ports->at(5) = _vel_port;
}

void
TriggerNode::run(RunContext& context)
{
	const BufferRef          midi_in  = _midi_in_port->buffer(0);
	LV2_Atom_Sequence* const seq      = midi_in->get<LV2_Atom_Sequence>();
	const BufferRef          midi_out = _midi_out_port->buffer(0);

	// Initialise output to the empty sequence
	midi_out->prepare_write(context);

	LV2_ATOM_SEQUENCE_FOREACH(seq, ev) {
		const int64_t  t    = ev->time.frames;
		const uint8_t* buf  = (const uint8_t*)LV2_ATOM_BODY(&ev->body);
		bool           emit = false;
		if (ev->body.type == _midi_in_port->bufs().uris().midi_MidiEvent &&
		    ev->body.size >= 3) {
			const FrameTime time = context.start() + t;
			switch (lv2_midi_message_type(buf)) {
			case LV2_MIDI_MSG_NOTE_ON:
				if (buf[2] == 0) {
					emit = note_off(context, buf[1], time);
				} else {
					emit = note_on(context, buf[1], buf[2], time);
				}
				break;
			case LV2_MIDI_MSG_NOTE_OFF:
				emit = note_off(context, buf[1], time);
				break;
			case LV2_MIDI_MSG_CONTROLLER:
				switch (buf[1]) {
				case LV2_MIDI_CTL_ALL_NOTES_OFF:
				case LV2_MIDI_CTL_ALL_SOUNDS_OFF:
					_gate_port->set_control_value(context, time, 0.0f);
					emit = true;
				}
			default:
				break;
			}
		}

		if (emit) {
			midi_out->append_event(t, &ev->body);
		}
	}
}

bool
TriggerNode::note_on(RunContext& context, uint8_t note_num, uint8_t velocity, FrameTime time)
{
	assert(time >= context.start() && time <= context.end());
	const uint32_t offset = time - context.start();

	if (_learning) {
		_note_port->set_control_value(context, time, (float)note_num);
		_note_port->force_monitor_update();
		_learning = false;
	}

	_note_port->update_values(offset, 0);
	if (note_num == lrintf(_note_port->buffer(0)->value_at(offset))) {
		_gate_port->set_control_value(context, time, 1.0f);
		_trig_port->set_control_value(context, time, 1.0f);
		_trig_port->set_control_value(context, time + 1, 0.0f);
		_vel_port->set_control_value(context, time, velocity / 127.0f);
		return true;
	}
	return false;
}

bool
TriggerNode::note_off(RunContext& context, uint8_t note_num, FrameTime time)
{
	assert(time >= context.start() && time <= context.end());
	const uint32_t offset = time - context.start();

	_note_port->update_values(offset, 0);
	if (note_num == lrintf(_note_port->buffer(0)->value_at(offset))) {
		_gate_port->set_control_value(context, time, 0.0f);
		return true;
	}

	return false;
}

} // namespace Internals
} // namespace Server
} // namespace Ingen
