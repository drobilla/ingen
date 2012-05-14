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
#include <string>

#include "ingen/shared/URIs.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "raul/log.hpp"
#include "raul/midi_events.h"

#include "AudioBuffer.hpp"
#include "InputPort.hpp"
#include "InternalPlugin.hpp"
#include "OutputPort.hpp"
#include "ProcessContext.hpp"
#include "ingen_config.h"
#include "internals/Trigger.hpp"
#include "util.hpp"

#define LOG(s) s << "[TriggerNode] "

using namespace std;

namespace Ingen {
namespace Server {
namespace Internals {

InternalPlugin* TriggerNode::internal_plugin(Shared::URIs& uris) {
	return new InternalPlugin(uris, NS_INTERNALS "Trigger", "trigger");
}

TriggerNode::TriggerNode(InternalPlugin*    plugin,
                         BufferFactory&     bufs,
                         const std::string& path,
                         bool               polyphonic,
                         PatchImpl*         parent,
                         SampleRate         srate)
	: NodeImpl(plugin, path, false, parent, srate)
	, _learning(false)
{
	const Ingen::Shared::URIs& uris = bufs.uris();
	_ports = new Raul::Array<PortImpl*>(5);

	_midi_in_port = new InputPort(bufs, this, "input", 0, 1,
	                              PortType::ATOM, uris.atom_Sequence, Raul::Atom());
	_midi_in_port->set_property(uris.lv2_name, bufs.forge().alloc("Input"));
	_ports->at(0) = _midi_in_port;

	_note_port = new InputPort(bufs, this, "note", 1, 1,
	                           PortType::CONTROL, 0, bufs.forge().make(60.0f));
	_note_port->set_property(uris.lv2_minimum, bufs.forge().make(0.0f));
	_note_port->set_property(uris.lv2_maximum, bufs.forge().make(127.0f));
	_note_port->set_property(uris.lv2_integer, bufs.forge().make(true));
	_note_port->set_property(uris.lv2_name, bufs.forge().alloc("Note"));
	_ports->at(1) = _note_port;

	_gate_port = new OutputPort(bufs, this, "gate", 2, 1,
	                            PortType::CV, 0, bufs.forge().make(0.0f));
	_gate_port->set_property(uris.lv2_portProperty, uris.lv2_toggled);
	_gate_port->set_property(uris.lv2_name, bufs.forge().alloc("Gate"));
	_ports->at(2) = _gate_port;

	_trig_port = new OutputPort(bufs, this, "trigger", 3, 1,
	                            PortType::CV, 0, bufs.forge().make(0.0f));
	_trig_port->set_property(uris.lv2_portProperty, uris.lv2_toggled);
	_trig_port->set_property(uris.lv2_name, bufs.forge().alloc("Trigger"));
	_ports->at(3) = _trig_port;

	_vel_port = new OutputPort(bufs, this, "velocity", 4, 1,
	                           PortType::CV, 0, bufs.forge().make(0.0f));
	_vel_port->set_property(uris.lv2_minimum, bufs.forge().make(0.0f));
	_vel_port->set_property(uris.lv2_maximum, bufs.forge().make(1.0f));
	_vel_port->set_property(uris.lv2_name, bufs.forge().alloc("Velocity"));
	_ports->at(4) = _vel_port;
}

void
TriggerNode::process(ProcessContext& context)
{
	NodeImpl::pre_process(context);

	Buffer* const      midi_in = _midi_in_port->buffer(0).get();
	LV2_Atom_Sequence* seq     = (LV2_Atom_Sequence*)midi_in->atom();
	LV2_ATOM_SEQUENCE_FOREACH(seq, ev) {
		const uint8_t* buf = (const uint8_t*)LV2_ATOM_BODY(&ev->body);
		if (ev->body.type == _midi_in_port->bufs().uris().midi_MidiEvent &&
		    ev->body.size >= 3) {
			const FrameTime time = context.start() + ev->time.frames;
			switch (buf[0] & 0xF0) {
			case MIDI_CMD_NOTE_ON:
				if (buf[2] == 0) {
					note_off(context, buf[1], time);
				} else {
					note_on(context, buf[1], buf[2], time);
				}
				break;
			case MIDI_CMD_NOTE_OFF:
				note_off(context, buf[1], time);
				break;
			case MIDI_CMD_CONTROL:
				if (buf[1] == MIDI_CTL_ALL_NOTES_OFF ||
				    buf[1] == MIDI_CTL_ALL_SOUNDS_OFF) {
					((AudioBuffer*)_gate_port->buffer(0).get())->set_value(
						0.0f, context.start(), time);
				}
			default:
				break;
			}
		}
	}

	NodeImpl::post_process(context);
}

void
TriggerNode::note_on(ProcessContext& context, uint8_t note_num, uint8_t velocity, FrameTime time)
{
	assert(time >= context.start() && time <= context.end());

	if (_learning) {
		// FIXME
		//_note_port->set_value(note_num);
		((AudioBuffer*)_note_port->buffer(0).get())->set_value(
			(float)note_num, context.start(), context.end());
		_note_port->broadcast_value(context, true);
		_learning = false;
	}

#ifdef RAUL_LOG_DEBUG
	LOG(debug) << path() << " note " << (int)note_num << " on @ " << time << endl;
#endif

	Sample filter_note = ((AudioBuffer*)_note_port->buffer(0).get())->value_at(0);
	if (filter_note >= 0.0 && filter_note < 127.0 && (note_num == (uint8_t)filter_note)) {
		((AudioBuffer*)_gate_port->buffer(0).get())->set_value(1.0f, context.start(), time);
		((AudioBuffer*)_trig_port->buffer(0).get())->set_value(1.0f, context.start(), time);
		((AudioBuffer*)_trig_port->buffer(0).get())->set_value(0.0f, context.start(), time + 1);
		((AudioBuffer*)_vel_port->buffer(0).get())->set_value(velocity / 127.0f, context.start(), time);
		assert(((AudioBuffer*)_trig_port->buffer(0).get())->data()[time - context.start()] == 1.0f);
	}
}

void
TriggerNode::note_off(ProcessContext& context, uint8_t note_num, FrameTime time)
{
	assert(time >= context.start() && time <= context.end());

	if (note_num == lrintf(((AudioBuffer*)_note_port->buffer(0).get())->value_at(0)))
		((AudioBuffer*)_gate_port->buffer(0).get())->set_value(0.0f, context.start(), time);
}

} // namespace Internals
} // namespace Server
} // namespace Ingen
