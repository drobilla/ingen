/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "internals/Note.hpp"

#include "BlockImpl.hpp"
#include "Buffer.hpp"
#include "BufferFactory.hpp"
#include "BufferRef.hpp"
#include "InputPort.hpp"
#include "InternalPlugin.hpp"
#include "OutputPort.hpp"
#include "PortType.hpp"
#include "RunContext.hpp"

#include "ingen/Atom.hpp"
#include "ingen/Forge.hpp"
#include "ingen/URI.hpp"
#include "ingen/URIs.hpp"
#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"
#include "lv2/midi/midi.h"
#include "raul/Array.hpp"
#include "raul/Maid.hpp"
#include "raul/Symbol.hpp"

#include <cassert>
#include <cmath>
#include <memory>
#include <utility>

// #define NOTE_DEBUG 1

namespace ingen {
namespace server {

class GraphImpl;

namespace internals {

InternalPlugin* NoteNode::internal_plugin(URIs& uris) {
	return new InternalPlugin(
		uris, URI(NS_INTERNALS "Note"), raul::Symbol("note"));
}

NoteNode::NoteNode(InternalPlugin*     plugin,
                   BufferFactory&      bufs,
                   const raul::Symbol& symbol,
                   bool                polyphonic,
                   GraphImpl*          parent,
                   SampleRate          srate)
	: InternalBlock(plugin, symbol, polyphonic, parent, srate)
	, _voices(bufs.maid().make_managed<Voices>(_polyphony))
	, _sustain(false)
{
	const ingen::URIs& uris = bufs.uris();
	_ports = bufs.maid().make_managed<Ports>(8);

	const Atom zero = bufs.forge().make(0.0f);
	const Atom one  = bufs.forge().make(1.0f);

	_midi_in_port = new InputPort(bufs, this, raul::Symbol("input"), 0, 1,
	                              PortType::ATOM, uris.atom_Sequence, Atom());
	_midi_in_port->set_property(uris.lv2_name, bufs.forge().alloc("Input"));
	_midi_in_port->set_property(uris.atom_supports,
	                            bufs.forge().make_urid(uris.midi_MidiEvent));
	_ports->at(0) = _midi_in_port;

	_freq_port = new OutputPort(bufs, this, raul::Symbol("frequency"), 1, _polyphony,
	                            PortType::ATOM, uris.atom_Sequence,
	                            bufs.forge().make(440.0f));
	_freq_port->set_property(uris.atom_supports, bufs.uris().atom_Float);
	_freq_port->set_property(uris.lv2_name, bufs.forge().alloc("Frequency"));
	_freq_port->set_property(uris.lv2_minimum, bufs.forge().make(16.0f));
	_freq_port->set_property(uris.lv2_maximum, bufs.forge().make(25088.0f));
	_ports->at(1) = _freq_port;

	_num_port = new OutputPort(bufs, this, raul::Symbol("number"), 1, _polyphony,
	                           PortType::ATOM, uris.atom_Sequence, zero);
	_num_port->set_property(uris.atom_supports, bufs.uris().atom_Float);
	_num_port->set_property(uris.lv2_minimum, zero);
	_num_port->set_property(uris.lv2_maximum, bufs.forge().make(127.0f));
	_num_port->set_property(uris.lv2_portProperty, uris.lv2_integer);
	_num_port->set_property(uris.lv2_name, bufs.forge().alloc("Number"));
	_ports->at(2) = _num_port;

	_vel_port = new OutputPort(bufs, this, raul::Symbol("velocity"), 2, _polyphony,
	                           PortType::ATOM, uris.atom_Sequence, zero);
	_vel_port->set_property(uris.atom_supports, bufs.uris().atom_Float);
	_vel_port->set_property(uris.lv2_minimum, zero);
	_vel_port->set_property(uris.lv2_maximum, one);
	_vel_port->set_property(uris.lv2_name, bufs.forge().alloc("Velocity"));
	_ports->at(3) = _vel_port;

	_gate_port = new OutputPort(bufs, this, raul::Symbol("gate"), 3, _polyphony,
	                            PortType::ATOM, uris.atom_Sequence, zero);
	_gate_port->set_property(uris.atom_supports, bufs.uris().atom_Float);
	_gate_port->set_property(uris.lv2_portProperty, uris.lv2_toggled);
	_gate_port->set_property(uris.lv2_name, bufs.forge().alloc("Gate"));
	_ports->at(4) = _gate_port;

	_trig_port = new OutputPort(bufs, this, raul::Symbol("trigger"), 4, _polyphony,
	                            PortType::ATOM, uris.atom_Sequence, zero);
	_trig_port->set_property(uris.atom_supports, bufs.uris().atom_Float);
	_trig_port->set_property(uris.lv2_portProperty, uris.lv2_toggled);
	_trig_port->set_property(uris.lv2_name, bufs.forge().alloc("Trigger"));
	_ports->at(5) = _trig_port;

	_bend_port = new OutputPort(bufs, this, raul::Symbol("bend"), 5, _polyphony,
	                            PortType::ATOM, uris.atom_Sequence, zero);
	_bend_port->set_property(uris.atom_supports, bufs.uris().atom_Float);
	_bend_port->set_property(uris.lv2_name, bufs.forge().alloc("Bender"));
	_bend_port->set_property(uris.lv2_default, zero);
	_bend_port->set_property(uris.lv2_minimum, bufs.forge().make(-1.0f));
	_bend_port->set_property(uris.lv2_maximum, one);
	_ports->at(6) = _bend_port;

	_pressure_port = new OutputPort(bufs, this, raul::Symbol("pressure"), 6, _polyphony,
	                            PortType::ATOM, uris.atom_Sequence, zero);
	_pressure_port->set_property(uris.atom_supports, bufs.uris().atom_Float);
	_pressure_port->set_property(uris.lv2_name, bufs.forge().alloc("Pressure"));
	_pressure_port->set_property(uris.lv2_default, zero);
	_pressure_port->set_property(uris.lv2_minimum, zero);
	_pressure_port->set_property(uris.lv2_maximum, one);
	_ports->at(7) = _pressure_port;
}

bool
NoteNode::prepare_poly(BufferFactory& bufs, uint32_t poly)
{
	if (!_polyphonic) {
		return true;
	}

	BlockImpl::prepare_poly(bufs, poly);

	if (_prepared_voices && poly <= _prepared_voices->size()) {
		return true;
	}

	_prepared_voices = bufs.maid().make_managed<Voices>(
		poly, *_voices, Voice());

	return true;
}

bool
NoteNode::apply_poly(RunContext& ctx, uint32_t poly)
{
	if (!BlockImpl::apply_poly(ctx, poly)) {
		return false;
	}

	if (_prepared_voices) {
		assert(_polyphony <= _prepared_voices->size());
		_voices = std::move(_prepared_voices);
	}
	assert(_polyphony <= _voices->size());

	return true;
}

void
NoteNode::run(RunContext& ctx)
{
	Buffer* const midi_in = _midi_in_port->buffer(0).get();
	auto*         seq     = midi_in->get<LV2_Atom_Sequence>();

	LV2_ATOM_SEQUENCE_FOREACH(seq, ev) {
		const auto* buf =
		    static_cast<const uint8_t*>(LV2_ATOM_BODY_CONST(&ev->body));

		const FrameTime time =
		    ctx.start() + static_cast<FrameTime>(ev->time.frames);

		if (ev->body.type == _midi_in_port->bufs().uris().midi_MidiEvent &&
		    ev->body.size >= 3) {
			switch (lv2_midi_message_type(buf)) {
			case LV2_MIDI_MSG_NOTE_ON:
				if (buf[2] == 0) {
					note_off(ctx, buf[1], time);
				} else {
					note_on(ctx, buf[1], buf[2], time);
				}
				break;
			case LV2_MIDI_MSG_NOTE_OFF:
				note_off(ctx, buf[1], time);
				break;
			case LV2_MIDI_MSG_CONTROLLER:
				switch (buf[1]) {
				case LV2_MIDI_CTL_ALL_NOTES_OFF:
				case LV2_MIDI_CTL_ALL_SOUNDS_OFF:
					all_notes_off(ctx, time);
					break;
				case LV2_MIDI_CTL_SUSTAIN:
					if (buf[2] > 63) {
						sustain_on(ctx, time);
					} else {
						sustain_off(ctx, time);
					}
					break;
				}
				break;
			case LV2_MIDI_MSG_BENDER:
				bend(ctx,
				     time,
				     ((((static_cast<uint16_t>(buf[2]) << 7) | buf[1]) -
				       8192.0f) /
				      8192.0f));
				break;
			case LV2_MIDI_MSG_CHANNEL_PRESSURE:
				channel_pressure(ctx, time, buf[1] / 127.0f);
				break;
			case LV2_MIDI_MSG_NOTE_PRESSURE:
				note_pressure(ctx, time, buf[1], buf[2] / 127.0f);
				break;
			default:
				break;
			}
		}
	}
}

static inline float
note_to_freq(uint8_t num)
{
	static const float A4 = 440.0f;
	return A4 * powf(2.0f, static_cast<float>(num - 57.0f) / 12.0f);
}

void
NoteNode::note_on(RunContext& ctx, uint8_t note_num, uint8_t velocity, FrameTime time)
{
	assert(time >= ctx.start() && time <= ctx.end());
	assert(note_num <= 127);

	Key*     key       = &_keys[note_num];
	Voice*   voice     = nullptr;
	uint32_t voice_num = 0;

	if (key->state != Key::State::OFF) {
		return;
	}

	// Look for free voices
	for (uint32_t i=0; i < _polyphony; ++i) {
		if ((*_voices)[i].state == Voice::State::FREE) {
			voice = &(*_voices)[i];
			voice_num = i;
			break;
		}
	}

	// If we didn't find a free one, steal the oldest
	if (voice == nullptr) {
		voice_num = 0;
		voice = &(*_voices)[0];
		FrameTime oldest_time = (*_voices)[0].time;
		for (uint32_t i=1; i < _polyphony; ++i) {
			if ((*_voices)[i].time < oldest_time) {
				voice = &(*_voices)[i];
				voice_num = i;
				oldest_time = voice->time;
			}
		}
	}
	assert(voice != nullptr);
	assert(voice == &(*_voices)[voice_num]);

	// Update stolen key, if applicable
	if (voice->state == Voice::State::ACTIVE) {
		assert(_keys[voice->note].state == Key::State::ON_ASSIGNED);
		assert(_keys[voice->note].voice == voice_num);
		_keys[voice->note].state = Key::State::ON_UNASSIGNED;
	}

	// Store key information for later reallocation on note off
	key->state = Key::State::ON_ASSIGNED;
	key->voice = voice_num;
	key->time  = time;

	// Check if we just triggered this voice at the same time
	// (Double note-on at the same sample on the same voice)
	const bool double_trigger = (voice->state == Voice::State::ACTIVE &&
	                             voice->time == time);

	// Trigger voice
	voice->state = Voice::State::ACTIVE;
	voice->note  = note_num;
	voice->time  = time;

	assert(_keys[voice->note].state == Key::State::ON_ASSIGNED);
	assert(_keys[voice->note].voice == voice_num);

	_freq_port->set_voice_value(ctx, voice_num, time, note_to_freq(note_num));
	_num_port->set_voice_value(ctx, voice_num, time, static_cast<float>(note_num));
	_vel_port->set_voice_value(ctx, voice_num, time, velocity / 127.0f);
	_gate_port->set_voice_value(ctx, voice_num, time, 1.0f);
	if (!double_trigger) {
		_trig_port->set_voice_value(ctx, voice_num, time, 1.0f);
		_trig_port->set_voice_value(ctx, voice_num, time + 1, 0.0f);
	}

	assert(key->state == Key::State::ON_ASSIGNED);
	assert(voice->state == Voice::State::ACTIVE);
	assert(key->voice == voice_num);
	assert((*_voices)[key->voice].note == note_num);
}

void
NoteNode::note_off(RunContext& ctx, uint8_t note_num, FrameTime time)
{
	assert(time >= ctx.start() && time <= ctx.end());

	Key* key = &_keys[note_num];

	if (key->state == Key::State::ON_ASSIGNED) {
		// Assigned key, turn off voice and key
		if ((*_voices)[key->voice].state == Voice::State::ACTIVE) {
			assert((*_voices)[key->voice].note == note_num);
			if ( ! _sustain) {
				free_voice(ctx, key->voice, time);
			} else {
				(*_voices)[key->voice].state = Voice::State::HOLDING;
			}
		}
	}

	key->state = Key::State::OFF;
}

void
NoteNode::free_voice(RunContext& ctx, uint32_t voice, FrameTime time)
{
	assert(time >= ctx.start() && time <= ctx.end());

	// Find a key to reassign to the freed voice (the newest, if there is one)
	Key*    replace_key     = nullptr;
	uint8_t replace_key_num = 0;

	for (uint8_t i = 0; i <= 127; ++i) {
		if (_keys[i].state == Key::State::ON_UNASSIGNED) {
			if (replace_key == nullptr || _keys[i].time > replace_key->time) {
				replace_key = &_keys[i];
				replace_key_num = i;
			}
		}
	}

	if (replace_key != nullptr) {  // Found a key to assign to freed voice
		assert(&_keys[replace_key_num] == replace_key);
		assert(replace_key->state == Key::State::ON_UNASSIGNED);

		// Change the freq but leave the gate high and don't retrigger
		_freq_port->set_voice_value(ctx, voice, time, note_to_freq(replace_key_num));
		_num_port->set_voice_value(ctx, voice, time, replace_key_num);

		replace_key->state = Key::State::ON_ASSIGNED;
		replace_key->voice = voice;
		_keys[(*_voices)[voice].note].state = Key::State::ON_UNASSIGNED;
		(*_voices)[voice].note = replace_key_num;
		(*_voices)[voice].state = Voice::State::ACTIVE;
	} else {
		// No new note for voice, deactivate (set gate low)
		_gate_port->set_voice_value(ctx, voice, time, 0.0f);
		(*_voices)[voice].state = Voice::State::FREE;
	}
}

void
NoteNode::all_notes_off(RunContext& ctx, FrameTime time)
{
	assert(time >= ctx.start() && time <= ctx.end());

	// FIXME: set all keys to Key::OFF?

	for (uint32_t i = 0; i < _polyphony; ++i) {
		_gate_port->set_voice_value(ctx, i, time, 0.0f);
		(*_voices)[i].state = Voice::State::FREE;
	}
}

void
NoteNode::sustain_on(RunContext&, FrameTime)
{
	_sustain = true;
}

void
NoteNode::sustain_off(RunContext& ctx, FrameTime time)
{
	assert(time >= ctx.start() && time <= ctx.end());

	_sustain = false;

	for (uint32_t i=0; i < _polyphony; ++i) {
		if ((*_voices)[i].state == Voice::State::HOLDING) {
			free_voice(ctx, i, time);
		}
	}
}

void
NoteNode::bend(RunContext& ctx, FrameTime time, float amount)
{
	_bend_port->set_control_value(ctx, time, amount);
}

void
NoteNode::note_pressure(RunContext& ctx, FrameTime time, uint8_t note_num, float amount)
{
	for (uint32_t i=0; i < _polyphony; ++i) {
		if ((*_voices)[i].state != Voice::State::FREE && (*_voices)[i].note == note_num) {
			_pressure_port->set_voice_value(ctx, i, time, amount);
			return;
		}
	}
}

void
NoteNode::channel_pressure(RunContext& ctx, FrameTime time, float amount)
{
	_pressure_port->set_control_value(ctx, time, amount);
}

} // namespace internals
} // namespace server
} // namespace ingen
