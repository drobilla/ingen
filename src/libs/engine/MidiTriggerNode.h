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
	MidiTriggerNode(const string& path, size_t poly, Patch* parent, samplerate srate, size_t buffer_size);

	void process(samplecount nframes);
	
	void note_on(uchar note_num, uchar velocity, samplecount offset);
	void note_off(uchar note_num, samplecount offset);

private:
	InputPort<MidiMessage>* _midi_in_port;
	InputPort<sample>*      _note_port;
	OutputPort<sample>*     _gate_port;
	OutputPort<sample>*     _trig_port;
	OutputPort<sample>*     _vel_port;
};


} // namespace Om

#endif // MIDITRIGGERNODE_H
