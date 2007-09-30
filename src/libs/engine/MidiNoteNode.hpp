/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
#include "types.hpp"
#include "NodeBase.hpp"
#include "MidiBuffer.hpp"


namespace Ingen {

class InputPort;
class OutputPort;


/** MIDI note input node.
 *
 * For pitched instruments like keyboard, etc.
 *
 * \ingroup engine
 */
class MidiNoteNode : public NodeBase
{
public:
	MidiNoteNode(const std::string& path, bool polyphonic, Patch* parent, SampleRate srate, size_t buffer_size);
	~MidiNoteNode();
	
	bool prepare_poly(uint32_t poly);
	bool apply_poly(Raul::Maid& maid, uint32_t poly);

	void process(ProcessContext& context, SampleCount nframes, FrameTime start, FrameTime end);
	
	void note_on(uchar note_num, uchar velocity, FrameTime time, SampleCount nframes, FrameTime start, FrameTime end);
	void note_off(uchar note_num, FrameTime time, SampleCount nframes, FrameTime start, FrameTime end);
	void all_notes_off(FrameTime time, SampleCount nframes, FrameTime start, FrameTime end);

	void sustain_on(FrameTime time, SampleCount nframes, FrameTime start, FrameTime end);
	void sustain_off(FrameTime time, SampleCount nframes, FrameTime start, FrameTime end);

private:
	/** Key, one for each key on the keyboard */
	struct Key {
		enum State { OFF, ON_ASSIGNED, ON_UNASSIGNED };
		Key() : state(OFF), voice(0), time(0) {}
		State state; uint32_t voice; SampleCount time;
	};

	/** Voice, one of these always exists for each voice */
	struct Voice {
		enum State { FREE, ACTIVE, HOLDING };
		Voice() : state(FREE), note(0) {}
		State state; uchar note; SampleCount time;
	};

	float note_to_freq(int num);
	void free_voice(uint32_t voice, FrameTime time, SampleCount nframes, FrameTime start, FrameTime end);

	Raul::Array<Voice>* _voices;
	Raul::Array<Voice>* _prepared_voices;
	Key                 _keys[128];
	bool                _sustain; ///< Whether or not hold pedal is depressed
	
	InputPort*  _midi_in_port;
	OutputPort* _freq_port;
	OutputPort* _vel_port;
	OutputPort* _gate_port;
	OutputPort* _trig_port;
};


} // namespace Ingen

#endif // MIDINOTENODE_H
