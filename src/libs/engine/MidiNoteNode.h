/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
 * 
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MIDINOTENODE_H
#define MIDINOTENODE_H

#include <string>
#include "InternalNode.h"
#include "types.h"

using std::string;

namespace Ingen {

class MidiMessage;
template <typename T> class InputPort;
template <typename T> class OutputPort;


/** MIDI note input node.
 *
 * For pitched instruments like keyboard, etc.
 *
 * \ingroup engine
 */
class MidiNoteNode : public InternalNode
{
public:
	MidiNoteNode(const string& path, size_t poly, Patch* parent, SampleRate srate, size_t buffer_size);
	~MidiNoteNode();

	void process(SampleCount nframes, FrameTime start, FrameTime end);
	
	void note_on(uchar note_num, uchar velocity, SampleCount nframes, FrameTime time, FrameTime start, FrameTime end);
	void note_off(uchar note_num, FrameTime time, SampleCount nframes, FrameTime start, FrameTime end);
	void all_notes_off(SampleCount nframes, FrameTime time, FrameTime start, FrameTime end);

	void sustain_on(FrameTime time, SampleCount nframes, FrameTime start, FrameTime end);
	void sustain_off(FrameTime time, SampleCount nframes, FrameTime start, FrameTime end);

private:
	/** Key, one for each key on the keyboard */
	struct Key {
		enum State { OFF, ON_ASSIGNED, ON_UNASSIGNED };
		Key() : state(OFF), voice(0), time(0) {}
		State state; size_t voice; SampleCount time;
	};

	/** Voice, one of these always exists for each voice */
	struct Voice {
		enum State { FREE, ACTIVE, HOLDING };
		Voice() : state(FREE), note(0) {}
		State state; uchar note; SampleCount time;
	};

	float note_to_freq(int num);
	void free_voice(size_t voice, FrameTime time, SampleCount nframes, FrameTime start, FrameTime end);

	Voice* _voices;
	Key    _keys[128];
	bool   _sustain;   ///< Whether or not hold pedal is depressed
	
	InputPort<MidiMessage>* _midi_in_port;
	OutputPort<Sample>*     _freq_port;
	OutputPort<Sample>*     _vel_port;
	OutputPort<Sample>*     _gate_port;
	OutputPort<Sample>*     _trig_port;
};


} // namespace Ingen

#endif // MIDINOTENODE_H
