/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

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
#include "ProcessContext.hpp"
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
	: BlockImpl(plugin, symbol, false, parent, srate)
	, _learning(false)
{
	const Ingen::URIs& uris = bufs.uris();
	_ports = new Raul::Array<PortImpl*>(5);

	_midi_in_port = new InputPort(bufs, this, Raul::Symbol("input"), 0, 1,
	                              PortType::ATOM, uris.atom_Sequence, Raul::Atom());
	_midi_in_port->set_property(uris.lv2_name, bufs.forge().alloc("Input"));
	_ports->at(0) = _midi_in_port;

	_note_port = new InputPort(bufs, this, Raul::Symbol("note"), 1, 1,
	                           PortType::CONTROL, 0, bufs.forge().make(60.0f));
	_note_port->set_property(uris.lv2_minimum, bufs.forge().make(0.0f));
	_note_port->set_property(uris.lv2_maximum, bufs.forge().make(127.0f));
	_note_port->set_property(uris.lv2_integer, bufs.forge().make(true));
	_note_port->set_property(uris.lv2_name, bufs.forge().alloc("Note"));
	_ports->at(1) = _note_port;

	_gate_port = new OutputPort(bufs, this, Raul::Symbol("gate"), 2, 1,
	                            PortType::CV, 0, bufs.forge().make(0.0f));
	_gate_port->set_property(uris.lv2_portProperty, uris.lv2_toggled);
	_gate_port->set_property(uris.lv2_name, bufs.forge().alloc("Gate"));
	_ports->at(2) = _gate_port;

	_trig_port = new OutputPort(bufs, this, Raul::Symbol("trigger"), 3, 1,
	                            PortType::CV, 0, bufs.forge().make(0.0f));
	_trig_port->set_property(uris.lv2_portProperty, uris.lv2_toggled);
	_trig_port->set_property(uris.lv2_name, bufs.forge().alloc("Trigger"));
	_ports->at(3) = _trig_port;

	_vel_port = new OutputPort(bufs, this, Raul::Symbol("velocity"), 4, 1,
	                           PortType::CV, 0, bufs.forge().make(0.0f));
	_vel_port->set_property(uris.lv2_minimum, bufs.forge().make(0.0f));
	_vel_port->set_property(uris.lv2_maximum, bufs.forge().make(1.0f));
	_vel_port->set_property(uris.lv2_name, bufs.forge().alloc("Velocity"));
	_ports->at(4) = _vel_port;
}

void
TriggerNode::process(ProcessContext& context)
{
	BlockImpl::pre_process(context);

	Buffer* const      midi_in = _midi_in_port->buffer(0).get();
	LV2_Atom_Sequence* seq     = (LV2_Atom_Sequence*)midi_in->atom();
	LV2_ATOM_SEQUENCE_FOREACH(seq, ev) {
		const uint8_t* buf = (const uint8_t*)LV2_ATOM_BODY(&ev->body);
		if (ev->body.type == _midi_in_port->bufs().uris().midi_MidiEvent &&
		    ev->body.size >= 3) {
			const FrameTime time = context.start() + ev->time.frames;
			switch (lv2_midi_message_type(buf)) {
			case LV2_MIDI_MSG_NOTE_ON:
				if (buf[2] == 0) {
					note_off(context, buf[1], time);
				} else {
					note_on(context, buf[1], buf[2], time);
				}
				break;
			case LV2_MIDI_MSG_NOTE_OFF:
				note_off(context, buf[1], time);
				break;
			case LV2_MIDI_MSG_CONTROLLER:
				switch (buf[1]) {
				case LV2_MIDI_CTL_ALL_NOTES_OFF:
				case LV2_MIDI_CTL_ALL_SOUNDS_OFF:
					_gate_port->set_control_value(context, time, 0.0f);
				}
			default:
				break;
			}
		}
	}

	BlockImpl::post_process(context);
}

void
TriggerNode::note_on(ProcessContext& context, uint8_t note_num, uint8_t velocity, FrameTime time)
{
	assert(time >= context.start() && time <= context.end());

	if (_learning) {
		// FIXME: not thread safe
		_note_port->set_value(context.engine().world()->forge().make(note_num));
		_note_port->set_control_value(context, time, note_num);
		_note_port->broadcast_value(context, true);
		_learning = false;
	}

	const Sample filter_note = _note_port->buffer(0)->value_at(0);
	if (filter_note >= 0.0 && filter_note < 127.0 && (note_num == (uint8_t)filter_note)) {
		_gate_port->set_control_value(context, time, 1.0f);
		_trig_port->set_control_value(context, time, 1.0f);
		_trig_port->set_control_value(context, time + 1, 0.0f);
		_vel_port->set_control_value(context, time, velocity / 127.0f);
		assert(_trig_port->buffer(0)->samples()[time - context.start()] == 1.0f);
	}
}

void
TriggerNode::note_off(ProcessContext& context, uint8_t note_num, FrameTime time)
{
	assert(time >= context.start() && time <= context.end());

	if (note_num == lrintf(_note_port->buffer(0)->value_at(0)))
		_gate_port->set_control_value(context, time, 0.0f);
}

} // namespace Internals
} // namespace Server
} // namespace Ingen
