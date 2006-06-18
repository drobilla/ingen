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

#ifndef MIDICONTROLNODE_H
#define MIDICONTROLNODE_H

#include <string>
#include "NodeBase.h"
#include "InternalNode.h"
using std::string;

namespace Om {
	
class MidiLearnResponseEvent;
class MidiMessage;
template <typename T> class InputPort;
template <typename T> class OutputPort;


/** MIDI control input node.
 *
 * Creating one of these nodes is how a user makes "MIDI Bindings".  Note that
 * this node will always be monophonic, the poly parameter is ignored.
 * 
 * \ingroup engine
 */
class MidiControlNode : public InternalNode
{
public:
	MidiControlNode(const string& path, size_t poly, Patch* parent, samplerate srate, size_t buffer_size);
	
	void run(size_t nframes);
	
	void control(uchar control_num, uchar val, samplecount offset);

	void learn(MidiLearnResponseEvent* ev) { _learning = true; _learn_event = ev; }

private:
	// Disallow copies (undefined)
	MidiControlNode(const MidiControlNode& copy);
	MidiControlNode& operator=(const MidiControlNode&);
	
	bool _learning;

	InputPort<MidiMessage>* _midi_in_port;
	InputPort<sample>*      _param_port;
	InputPort<sample>*      _log_port;
	InputPort<sample>*      _min_port;
	InputPort<sample>*      _max_port;
	OutputPort<sample>*     _control_port;
	OutputPort<sample>*     _audio_port;

	MidiLearnResponseEvent* _learn_event;
};


} // namespace Om

#endif // MIDICONTROLNODE_H
