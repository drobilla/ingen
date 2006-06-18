/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "MidiNoteNode.h"
#include <cmath>
#include <iostream>
#include "MidiMessage.h"
#include "InputPort.h"
#include "OutputPort.h"
#include "Plugin.h"
#include "Array.h"
#include "Om.h"
#include "OmApp.h"
#include "AudioDriver.h"
#include "util.h"
#include "midi.h"

using std::cerr; using std::cout; using std::endl;


namespace Om {


MidiNoteNode::MidiNoteNode(const string& path, size_t poly, Patch* parent, samplerate srate, size_t buffer_size)
: InternalNode(path, poly, parent, srate, buffer_size),
  _voices(new Voice[poly]),
  _sustain(false)
{
	_num_ports = 5;
	_ports = new Array<Port*>(_num_ports);
	
	_midi_in_port = new InputPort<MidiMessage>(this, "DataType::MIDI In", 0, 1, DataType::MIDI, _buffer_size);
	_ports->at(0) = _midi_in_port;

	_freq_port = new OutputPort<sample>(this, "Frequency", 1, poly, DataType::FLOAT, _buffer_size);
	//	new PortInfo("Frequency", AUDIO, OUTPUT, 440, 0, 99999), _buffer_size);
	_ports->at(1) = _freq_port;
	
	_vel_port = new OutputPort<sample>(this, "Velocity", 2, poly, DataType::FLOAT, _buffer_size);
	//	new PortInfo("Velocity", AUDIO, OUTPUT, 0, 0, 1), _buffer_size);
	_ports->at(2) = _vel_port;
	
	_gate_port = new OutputPort<sample>(this, "Gate", 3, poly, DataType::FLOAT, _buffer_size);
	//	new PortInfo("Gate", AUDIO, OUTPUT, 0, 0, 1), _buffer_size);
	_ports->at(3) = _gate_port;
	
	_trig_port = new OutputPort<sample>(this, "Trigger", 4, poly, DataType::FLOAT, _buffer_size);
	//	new PortInfo("Trigger", AUDIO, OUTPUT, 0, 0, 1), _buffer_size);
	_ports->at(4) = _trig_port;
	
	_plugin.type(Plugin::Internal);
	_plugin.plug_label("note_in");
	_plugin.name("Om Note Node (MIDI, OSC)");
}


MidiNoteNode::~MidiNoteNode()
{
	delete[] _voices;
}


void
MidiNoteNode::run(size_t nframes)
{
	InternalNode::run(nframes);
	
	MidiMessage ev;
	
	for (size_t i=0; i < _midi_in_port->buffer(0)->filled_size(); ++i) {
		ev = _midi_in_port->buffer(0)->value_at(i);

		switch (ev.buffer[0] & 0xF0) {
		case MIDI_CMD_NOTE_ON:
			if (ev.buffer[2] == 0)
				note_off(ev.buffer[1], ev.time);
			else
				note_on(ev.buffer[1], ev.buffer[2], ev.time);
			break;
		case MIDI_CMD_NOTE_OFF:
			note_off(ev.buffer[1], ev.time);
			break;
		case MIDI_CMD_CONTROL:
			switch (ev.buffer[1]) {
			case MIDI_CTL_ALL_NOTES_OFF:
			case MIDI_CTL_ALL_SOUNDS_OFF:
				all_notes_off(ev.time);
				break;
			case MIDI_CTL_SUSTAIN:
				if (ev.buffer[2] > 63)
					sustain_on();
				else
					sustain_off(ev.time);
				break;
			case MIDI_CMD_BENDER:

				break;
			}
		default:
			break;
		}
	}
}


void
MidiNoteNode::note_on(uchar note_num, uchar velocity, samplecount offset)
{
	const jack_nframes_t time_stamp = om->audio_driver()->time_stamp();

	assert(offset < _buffer_size);
	assert(note_num <= 127);

	Key*   key        = &_keys[note_num];
	Voice* voice      = NULL;
	size_t voice_num  = 0;
	
	// Look for free voices
	for (size_t i=0; i < _poly; ++i) {
		if (_voices[i].state == Voice::Voice::FREE) {
			voice = &_voices[i];
			voice_num = i;
			break;
		}
	}

	// If we didn't find a free one, steal the oldest
	if (voice == NULL) {
		voice_num = 0;
		voice = &_voices[0];
		jack_nframes_t oldest_time = _voices[0].time;
		for (size_t i=1; i < _poly; ++i) {
			if (_voices[i].time < oldest_time) {
				voice = &_voices[i];
				voice_num = i;
				oldest_time = voice->time;
			}
		}
	}		
	assert(voice != NULL);
	assert(voice == &_voices[voice_num]);

	//cerr << "[MidiNoteNode] Note on.  Key " << (int)note_num << ", Voice " << voice_num << endl;
	
	// Update stolen key, if applicable
	if (voice->state == Voice::Voice::ACTIVE) {
		assert(_keys[voice->note].voice == voice_num);
		assert(_keys[voice->note].state == Key::ON_ASSIGNED);
		_keys[voice->note].state = Key::Key::ON_UNASSIGNED;
		//cerr << "[MidiNoteNode] Stole voice " << voice_num << endl;
	}
	
	// Store key information for later reallocation on note off
	key->state = Key::Key::ON_ASSIGNED;
	key->voice = voice_num;
	key->time  = time_stamp;

	// Trigger voice
	voice->state = Voice::Voice::ACTIVE;
	voice->note  = note_num;
	voice->time  = time_stamp;
	
	assert(_keys[voice->note].state == Key::Key::ON_ASSIGNED);
	assert(_keys[voice->note].voice == voice_num);
	
	// one-sample jitter hack to avoid having to deal with trigger sample "next time"
	if (offset == (samplecount)(_buffer_size-1))
		--offset;
	
	_freq_port->buffer(voice_num)->set(note_to_freq(note_num), offset);
	_vel_port->buffer(voice_num)->set(velocity/127.0, offset);
	_gate_port->buffer(voice_num)->set(1.0f, offset);
	
	// trigger (one sample)
	_trig_port->buffer(voice_num)->set(1.0f, offset, offset);
	_trig_port->buffer(voice_num)->set(0.0f, offset+1);

	assert(key->state == Key::Key::ON_ASSIGNED);
	assert(voice->state == Voice::Voice::ACTIVE);
	assert(key->voice == voice_num);
	assert(_voices[key->voice].note == note_num);
}


void
MidiNoteNode::note_off(uchar note_num, samplecount offset)
{
	assert(offset < _buffer_size);

	Key* key = &_keys[note_num];

	if (key->state == Key::ON_ASSIGNED) {
		// Assigned key, turn off voice and key
		assert(_voices[key->voice].state == Voice::ACTIVE);
		assert(_voices[key->voice].note == note_num);
		key->state = Key::OFF;

		if ( ! _sustain)
			free_voice(key->voice, offset);
		else
			_voices[key->voice].state = Voice::HOLDING;
	}

	key->state = Key::OFF;
}

	
void
MidiNoteNode::free_voice(size_t voice, samplecount offset)
{
	// Find a key to reassign to the freed voice (the newest, if there is one)
	Key*  replace_key     = NULL;
	uchar replace_key_num = 0;

	for (uchar i = 0; i <= 127; ++i) {
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
		_freq_port->buffer(voice)->set(note_to_freq(replace_key_num), offset);

		replace_key->state = Key::ON_ASSIGNED;
		replace_key->voice = voice;
		_keys[_voices[voice].note].state = Key::ON_UNASSIGNED;
		_voices[voice].note = replace_key_num;
		_voices[voice].state = Voice::ACTIVE;
	} else {
		// No new note for voice, deactivate (set gate low)
		//cerr << "[MidiNoteNode] Note off. Key " << (int)note_num << ", Voice " << voice << " Killed" << endl;
		_gate_port->buffer(voice)->set(0.0f, offset);
		_voices[voice].state = Voice::FREE;
	}
}


void
MidiNoteNode::all_notes_off(samplecount offset)
{
	//cerr << "Note off starting at sample " << offset << endl;
	assert(offset < _buffer_size);

	// FIXME: set all keys to Key::OFF?
	
	for (size_t i=0; i < _poly; ++i) {
		_gate_port->buffer(i)->set(0.0f, offset);
		_voices[i].state = Voice::FREE;
	}
}


float
MidiNoteNode::note_to_freq(int num)
{
	static const float A4 = 440.0f;
	if (num >= 0 && num <= 119)
		return A4 * powf(2.0f, (float)(num - 57.0f) / 12.0f);
	return 1.0f;  // Some LADSPA plugins don't like freq=0
}


void
MidiNoteNode::sustain_on()
{
	_sustain = true;
}


void
MidiNoteNode::sustain_off(samplecount offset)
{
	_sustain = false;
	
	for (size_t i=0; i < _poly; ++i)
		if (_voices[i].state == Voice::HOLDING)
			free_voice(i, offset);
}


} // namespace Om

