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

#ifndef MIDILEARNEVENT_H
#define MIDILEARNEVENT_H

#include "QueuedEvent.h"
#include "MidiControlNode.h"
#include "types.h"
#include <string>
using std::string;

namespace Om {

class Node;
class ControlChangeEvent;


/** Response event for a MIDI learn.
 *
 * This is a trivial event that sends a control change in it's post_process
 * method (used by MidiLearnEvent to notify clients when the learn happens)
 */
class MidiLearnResponseEvent : public Event
{
public:
	MidiLearnResponseEvent(const string& port_path)
	: Event(CountedPtr<Responder>(NULL)),
	  m_port_path(port_path),
	  m_value(0.0f)
	{}
	
	void set_value(sample val) { m_value = val; }
	void post_process();
	
private:
	string m_port_path;
	sample m_value;
};



/** A MIDI learn event.
 *
 * This creates a MidiLearnResponseEvent and passes it to the learning node, which
 * will push it to the post-processor once the learn happens in order to reply
 * to the client with the new port (learned controller) value.
 *
 * \ingroup engine
 */
class MidiLearnEvent : public QueuedEvent
{
public:
	MidiLearnEvent(CountedPtr<Responder> responder, const string& node_path);
	
	void pre_process();
	void execute(samplecount offset);
	void post_process();

private:
	string  m_node_path;
	Node*   m_node;
	
	/// Event to respond with when learned
	MidiLearnResponseEvent* m_response_event;
};


} // namespace Om

#endif // MIDILEARNEVENT_H
