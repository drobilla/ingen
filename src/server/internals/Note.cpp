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

#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/shared/URIs.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "raul/Array.hpp"
#include "raul/Maid.hpp"
#include "raul/log.hpp"
#include "raul/midi_events.h"

#include "AudioBuffer.hpp"
#include "Driver.hpp"
#include "InputPort.hpp"
#include "InternalPlugin.hpp"
#include "OutputPort.hpp"
#include "PatchImpl.hpp"
#include "ProcessContext.hpp"
#include "ingen_config.h"
#include "internals/Note.hpp"
#include "util.hpp"

#define LOG(s) s << "[NoteNode] "

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {
namespace Internals {

InternalPlugin* NoteNode::internal_plugin(Shared::URIs& uris) {
	return new InternalPlugin(uris, NS_INTERNALS "Note", "note");
}

NoteNode::NoteNode(
		InternalPlugin*    plugin,
		BufferFactory&     bufs,
		const std::string& path,
		bool               polyphonic,
		PatchImpl*         parent,
		SampleRate         srate)
	: NodeImpl(plugin, path, polyphonic, parent, srate)
	, _voices(new Raul::Array<Voice>(_polyphony))
	, _prepared_voices(NULL)
	, _sustain(false)
{
	const Ingen::Shared::URIs& uris = bufs.uris();
	_ports = new Raul::Array<PortImpl*>(5);

	_midi_in_port = new InputPort(bufs, this, "input", 0, 1,
	                              PortType::ATOM, uris.atom_Sequence, Raul::Atom());
	_midi_in_port->set_property(uris.lv2_name, bufs.forge().alloc("Input"));
	_ports->at(0) = _midi_in_port;

	_freq_port = new OutputPort(bufs, this, "frequency", 1, _polyphony,
	                            PortType::AUDIO, 0, bufs.forge().make(440.0f));
	_freq_port->set_property(uris.lv2_name, bufs.forge().alloc("Frequency"));
	_ports->at(1) = _freq_port;

	_vel_port = new OutputPort(bufs, this, "velocity", 2, _polyphony,
	                           PortType::AUDIO, 0, bufs.forge().make(0.0f));
	_vel_port->set_property(uris.lv2_minimum, bufs.forge().make(0.0f));
	_vel_port->set_property(uris.lv2_maximum, bufs.forge().make(1.0f));
	_vel_port->set_property(uris.lv2_name, bufs.forge().alloc("Velocity"));
	_ports->at(2) = _vel_port;

	_gate_port = new OutputPort(bufs, this, "gate", 3, _polyphony,
	                            PortType::AUDIO, 0, bufs.forge().make(0.0f));
	_gate_port->set_property(uris.lv2_portProperty, uris.lv2_toggled);
	_gate_port->set_property(uris.lv2_name, bufs.forge().alloc("Gate"));
	_ports->at(3) = _gate_port;

	_trig_port = new OutputPort(bufs, this, "trigger", 4, _polyphony,
	                            PortType::AUDIO, 0, bufs.forge().make(0.0f));
	_trig_port->set_property(uris.lv2_portProperty, uris.lv2_toggled);
	_trig_port->set_property(uris.lv2_name, bufs.forge().alloc("Trigger"));
	_ports->at(4) = _trig_port;
}

NoteNode::~NoteNode()
{
	delete _voices;
}

bool
NoteNode::prepare_poly(BufferFactory& bufs, uint32_t poly)
{
	if (!_polyphonic)
		return true;

	NodeImpl::prepare_poly(bufs, poly);

	if (_prepared_voices && poly <= _prepared_voices->size())
		return true;

	_prepared_voices = new Raul::Array<Voice>(poly, *_voices, Voice());

	return true;
}

bool
NoteNode::apply_poly(Raul::Maid& maid, uint32_t poly)
{
	if (!NodeImpl::apply_poly(maid, poly))
		return false;

	if (_prepared_voices) {
		assert(_polyphony <= _prepared_voices->size());
		maid.push(_voices);
		_voices = _prepared_voices;
		_prepared_voices = NULL;
	}
	assert(_polyphony <= _voices->size());

	return true;
}

void
NoteNode::process(ProcessContext& context)
{
	NodeImpl::pre_process(context);

	Buffer* const      midi_in = _midi_in_port->buffer(0).get();
	LV2_Atom_Sequence* seq     = (LV2_Atom_Sequence*)midi_in->atom();
	LV2_ATOM_SEQUENCE_FOREACH(seq, ev) {
		const uint8_t*  buf  = (const uint8_t*)LV2_ATOM_BODY(&ev->body);
		const FrameTime time = context.start() + (FrameTime)ev->time.frames;
		if (ev->body.type == _midi_in_port->bufs().uris().midi_MidiEvent &&
		    ev->body.size >= 3) {
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
				switch (buf[1]) {
				case MIDI_CTL_ALL_NOTES_OFF:
				case MIDI_CTL_ALL_SOUNDS_OFF:
					all_notes_off(context, time);
					break;
				case MIDI_CTL_SUSTAIN:
					if (buf[2] > 63) {
						sustain_on(context, time);
					} else {
						sustain_off(context, time);
					}
					break;
				case MIDI_CMD_BENDER:
					// ?
					break;
				default:
					//warn << "Ignored controller " << buf[1] << endl;
					break;
				}
				break;
			default:
				//fprintf(stderr, "Unknown (size %d) MIDI event %X\n", size, buf[0]);
				break;
			}
		} else {
			//fprintf(stderr, "Unknown (size %d) MIDI event %X\n", size, buf[0]);
		}
	}

	NodeImpl::post_process(context);
}

void
NoteNode::note_on(ProcessContext& context, uint8_t note_num, uint8_t velocity, FrameTime time)
{
	assert(time >= context.start() && time <= context.end());
	assert(note_num <= 127);

	Key*   key         = &_keys[note_num];
	Voice* voice       = NULL;
	uint32_t voice_num = 0;

	if (key->state != Key::OFF) {
#ifdef RAUL_LOG_DEBUG
		LOG(debug) << "Double midi note received" << endl;
#endif
		return;
	}

	// Look for free voices
	for (uint32_t i=0; i < _polyphony; ++i) {
		if ((*_voices)[i].state == Voice::Voice::FREE) {
			voice = &(*_voices)[i];
			voice_num = i;
			break;
		}
	}

	// If we didn't find a free one, steal the oldest
	if (voice == NULL) {
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
	assert(voice != NULL);
	assert(voice == &(*_voices)[voice_num]);

#ifdef RAUL_LOG_DEBUG
	LOG(debug) << "Note " << (int)note_num << " on @ " << time
		<< ". Voice " << voice_num << " / " << _polyphony << endl;
#endif

	// Update stolen key, if applicable
	if (voice->state == Voice::Voice::ACTIVE) {
		assert(_keys[voice->note].state == Key::ON_ASSIGNED);
		assert(_keys[voice->note].voice == voice_num);
		_keys[voice->note].state = Key::Key::ON_UNASSIGNED;
#ifdef RAUL_LOG_DEBUG
		LOG(debug) << "Stole voice " << voice_num << endl;
#endif
	}

	// Store key information for later reallocation on note off
	key->state = Key::Key::ON_ASSIGNED;
	key->voice = voice_num;
	key->time  = time;

	// Trigger voice
	voice->state = Voice::Voice::ACTIVE;
	voice->note  = note_num;
	voice->time  = time;

	assert(_keys[voice->note].state == Key::Key::ON_ASSIGNED);
	assert(_keys[voice->note].voice == voice_num);

	((AudioBuffer*)_freq_port->buffer(voice_num).get())->set_value(
			note_to_freq(note_num), context.start(), time);
	((AudioBuffer*)_vel_port->buffer(voice_num).get())->set_value(
			velocity/127.0, context.start(), time);
	((AudioBuffer*)_gate_port->buffer(voice_num).get())->set_value(
			1.0f, context.start(), time);

	// trigger (one sample)
	((AudioBuffer*)_trig_port->buffer(voice_num).get())->set_value(
			1.0f, context.start(), time);
	((AudioBuffer*)_trig_port->buffer(voice_num).get())->set_value(
			0.0f, context.start(), time + 1);

	assert(key->state == Key::Key::ON_ASSIGNED);
	assert(voice->state == Voice::Voice::ACTIVE);
	assert(key->voice == voice_num);
	assert((*_voices)[key->voice].note == note_num);
}

void
NoteNode::note_off(ProcessContext& context, uint8_t note_num, FrameTime time)
{
	assert(time >= context.start() && time <= context.end());

	Key* key = &_keys[note_num];

#ifdef RAUL_LOG_DEBUG
	debug << "Note " << (int)note_num << " off @ " << time << endl;
#endif

	if (key->state == Key::ON_ASSIGNED) {
		// Assigned key, turn off voice and key
		if ((*_voices)[key->voice].state == Voice::ACTIVE) {
			assert((*_voices)[key->voice].note == note_num);

			if ( ! _sustain) {
#ifdef RAUL_LOG_DEBUG
				debug << "Free voice " << key->voice << endl;
#endif
				free_voice(context, key->voice, time);
			} else {
#ifdef RAUL_LOG_DEBUG
				debug << "Hold voice " << key->voice << endl;
#endif
				(*_voices)[key->voice].state = Voice::HOLDING;
			}

		} else {
#ifdef RAUL_LOG_DEBUG
			debug << "WARNING: Assigned key, but voice not active" << endl;
#endif
		}
	}

	key->state = Key::OFF;
}

void
NoteNode::free_voice(ProcessContext& context, uint32_t voice, FrameTime time)
{
	assert(time >= context.start() && time <= context.end());

	// Find a key to reassign to the freed voice (the newest, if there is one)
	Key*    replace_key     = NULL;
	uint8_t replace_key_num = 0;

	for (uint8_t i = 0; i <= 127; ++i) {
		if (_keys[i].state == Key::ON_UNASSIGNED) {
			if (replace_key == NULL || _keys[i].time > replace_key->time) {
				replace_key = &_keys[i];
				replace_key_num = i;
			}
		}
	}

	if (replace_key != NULL) {  // Found a key to assign to freed voice
		assert(&_keys[replace_key_num] == replace_key);
		assert(replace_key->state == Key::ON_UNASSIGNED);

		// Change the freq but leave the gate high and don't retrigger
		((AudioBuffer*)_freq_port->buffer(voice).get())->set_value(note_to_freq(replace_key_num), context.start(), time);

		replace_key->state = Key::ON_ASSIGNED;
		replace_key->voice = voice;
		_keys[(*_voices)[voice].note].state = Key::ON_UNASSIGNED;
		(*_voices)[voice].note = replace_key_num;
		(*_voices)[voice].state = Voice::ACTIVE;
	} else {
		// No new note for voice, deactivate (set gate low)
#ifdef RAUL_LOG_DEBUG
		LOG(debug) << "Note off: key " << (*_voices)[voice].note << " voice " << voice << endl;
#endif
		((AudioBuffer*)_gate_port->buffer(voice).get())->set_value(0.0f, context.start(), time);
		(*_voices)[voice].state = Voice::FREE;
	}
}

void
NoteNode::all_notes_off(ProcessContext& context, FrameTime time)
{
	assert(time >= context.start() && time <= context.end());

#ifdef RAUL_LOG_DEBUG
	LOG(debug) << "All notes off @ " << time << endl;
#endif

	// FIXME: set all keys to Key::OFF?

	for (uint32_t i = 0; i < _polyphony; ++i) {
		((AudioBuffer*)_gate_port->buffer(i).get())->set_value(0.0f, context.start(), time);
		(*_voices)[i].state = Voice::FREE;
	}
}

float
NoteNode::note_to_freq(int num)
{
	static const float A4 = 440.0f;
	if (num >= 0 && num <= 119)
		return A4 * powf(2.0f, (float)(num - 57.0f) / 12.0f);
	return 1.0f;  // Frequency of zero causes numerical problems...
}

void
NoteNode::sustain_on(ProcessContext& context, FrameTime time)
{
	_sustain = true;
}

void
NoteNode::sustain_off(ProcessContext& context, FrameTime time)
{
	assert(time >= context.start() && time <= context.end());

	_sustain = false;

	for (uint32_t i=0; i < _polyphony; ++i)
		if ((*_voices)[i].state == Voice::HOLDING)
			free_voice(context, i, time);
}

} // namespace Internals
} // namespace Server
} // namespace Ingen

