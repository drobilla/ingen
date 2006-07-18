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


#ifndef MIDITRIGGERNODE_H
#define MIDITRIGGERNODE_H

#include <string>
#include "InternalNode.h"

using std::string;

namespace Om {

class MidiMessage;
template <typename T> class InputPort;
template <typename T> class OutputPort;


/** MIDI trigger input node.
 *
 * Just has a gate,  for drums etc.  A control port is used to select
 * which note number is responded to.
 *
 * Note that this node is always monophonic, the poly parameter is ignored.
 * (Should that change?)
 *
 * \ingroup engine
 */
class MidiTriggerNode : public InternalNode
{
public:
	MidiTriggerNode(const string& path, size_t poly, Patch* parent, SampleRate srate, size_t buffer_size);

	void process(SampleCount nframes);
	
	void note_on(uchar note_num, uchar velocity, SampleCount offset);
	void note_off(uchar note_num, SampleCount offset);

private:
	InputPort<MidiMessage>* _midi_in_port;
	InputPort<Sample>*      _note_port;
	OutputPort<Sample>*     _gate_port;
	OutputPort<Sample>*     _trig_port;
	OutputPort<Sample>*     _vel_port;
};


} // namespace Om

#endif // MIDITRIGGERNODE_H
