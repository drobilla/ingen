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
#include "PortInfo.h"
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
  m_voices(new Voice[poly]),
  m_sustain(false)
{
	m_num_ports = 5;
	m_ports.alloc(m_num_ports);
	
	m_midi_in_port = new InputPort<MidiMessage>(this, "MIDI In", 0, 1,
		new PortInfo("MIDI In", MIDI, INPUT), m_buffer_size);
	m_ports.at(0) = m_midi_in_port;

	m_freq_port = new OutputPort<sample>(this, "Frequency", 1, poly,
		new PortInfo("Frequency", AUDIO, OUTPUT, 440, 0, 99999), m_buffer_size);
	m_ports.at(1) = m_freq_port;
	
	m_vel_port = new OutputPort<sample>(this, "Velocity", 2, poly,
		new PortInfo("Velocity", AUDIO, OUTPUT, 0, 0, 1), m_buffer_size);
	m_ports.at(2) = m_vel_port;
	
	m_gate_port = new OutputPort<sample>(this, "Gate", 3, poly,
		new PortInfo("Gate", AUDIO, OUTPUT, 0, 0, 1), m_buffer_size);
	m_ports.at(3) = m_gate_port;
	
	m_trig_port = new OutputPort<sample>(this, "Trigger", 4, poly,
		new PortInfo("Trigger", AUDIO, OUTPUT, 0, 0, 1), m_buffer_size);
	m_ports.at(4) = m_trig_port;
	
	m_plugin.type(Plugin::Internal);
	m_plugin.plug_label("note_in");
	m_plugin.name("Om Note Node (MIDI, OSC)");
}


MidiNoteNode::~MidiNoteNode()
{
	delete[] m_voices;
}


void
MidiNoteNode::run(size_t nframes)
{
	InternalNode::run(nframes);
	
	MidiMessage ev;
	
	for (size_t i=0; i < m_midi_in_port->buffer(0)->filled_size(); ++i) {
		ev = m_midi_in_port->buffer(0)->value_at(i);

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

	assert(offset < m_buffer_size);
	assert(note_num <= 127);

	Key*   key        = &m_keys[note_num];
	Voice* voice      = NULL;
	size_t voice_num  = 0;
	
	// Look for free voices
	for (size_t i=0; i < m_poly; ++i) {
		if (m_voices[i].state == Voice::Voice::FREE) {
			voice = &m_voices[i];
			voice_num = i;
			break;
		}
	}

	// If we didn't find a free one, steal the oldest
	if (voice == NULL) {
		voice_num = 0;
		voice = &m_voices[0];
		jack_nframes_t oldest_time = m_voices[0].time;
		for (size_t i=1; i < m_poly; ++i) {
			if (m_voices[i].time < oldest_time) {
				voice = &m_voices[i];
				voice_num = i;
				oldest_time = voice->time;
			}
		}
	}		
	assert(voice != NULL);
	assert(voice == &m_voices[voice_num]);

	//cerr << "[MidiNoteNode] Note on.  Key " << (int)note_num << ", Voice " << voice_num << endl;
	
	// Update stolen key, if applicable
	if (voice->state == Voice::Voice::ACTIVE) {
		assert(m_keys[voice->note].voice == voice_num);
		assert(m_keys[voice->note].state == Key::ON_ASSIGNED);
		m_keys[voice->note].state = Key::Key::ON_UNASSIGNED;
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
	
	assert(m_keys[voice->note].state == Key::Key::ON_ASSIGNED);
	assert(m_keys[voice->note].voice == voice_num);
	
	// one-sample jitter hack to avoid having to deal with trigger sample "next time"
	if (offset == (samplecount)(m_buffer_size-1))
		--offset;
	
	m_freq_port->buffer(voice_num)->set(note_to_freq(note_num), offset);
	m_vel_port->buffer(voice_num)->set(velocity/127.0, offset);
	m_gate_port->buffer(voice_num)->set(1.0f, offset);
	
	// trigger (one sample)
	m_trig_port->buffer(voice_num)->set(1.0f, offset, offset);
	m_trig_port->buffer(voice_num)->set(0.0f, offset+1);

	assert(key->state == Key::Key::ON_ASSIGNED);
	assert(voice->state == Voice::Voice::ACTIVE);
	assert(key->voice == voice_num);
	assert(m_voices[key->voice].note == note_num);
}


void
MidiNoteNode::note_off(uchar note_num, samplecount offset)
{
	assert(offset < m_buffer_size);

	Key* key = &m_keys[note_num];

	if (key->state == Key::ON_ASSIGNED) {
		// Assigned key, turn off voice and key
		assert(m_voices[key->voice].state == Voice::ACTIVE);
		assert(m_voices[key->voice].note == note_num);
		key->state = Key::OFF;

		if ( ! m_sustain)
			free_voice(key->voice, offset);
		else
			m_voices[key->voice].state = Voice::HOLDING;
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
		if (m_keys[i].state == Key::ON_UNASSIGNED) {
			if (replace_key == NULL || m_keys[i].time > replace_key->time) {
				replace_key = &m_keys[i];
				replace_key_num = i;
			}
		}
	}

	if (replace_key != NULL) {  // Found a key to assign to freed voice
		assert(&m_keys[replace_key_num] == replace_key);
		assert(replace_key->state == Key::ON_UNASSIGNED);
		
		// Change the freq but leave the gate high and don't retrigger
		m_freq_port->buffer(voice)->set(note_to_freq(replace_key_num), offset);

		replace_key->state = Key::ON_ASSIGNED;
		replace_key->voice = voice;
		m_keys[m_voices[voice].note].state = Key::ON_UNASSIGNED;
		m_voices[voice].note = replace_key_num;
		m_voices[voice].state = Voice::ACTIVE;
	} else {
		// No new note for voice, deactivate (set gate low)
		//cerr << "[MidiNoteNode] Note off. Key " << (int)note_num << ", Voice " << voice << " Killed" << endl;
		m_gate_port->buffer(voice)->set(0.0f, offset);
		m_voices[voice].state = Voice::FREE;
	}
}


void
MidiNoteNode::all_notes_off(samplecount offset)
{
	//cerr << "Note off starting at sample " << offset << endl;
	assert(offset < m_buffer_size);

	// FIXME: set all keys to Key::OFF?
	
	for (size_t i=0; i < m_poly; ++i) {
		m_gate_port->buffer(i)->set(0.0f, offset);
		m_voices[i].state = Voice::FREE;
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
	m_sustain = true;
}


void
MidiNoteNode::sustain_off(samplecount offset)
{
	m_sustain = false;
	
	for (size_t i=0; i < m_poly; ++i)
		if (m_voices[i].state == Voice::HOLDING)
			free_voice(i, offset);
}


} // namespace Om

