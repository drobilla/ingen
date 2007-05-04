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

#ifndef MIDILEARNEVENT_H
#define MIDILEARNEVENT_H

#include "QueuedEvent.h"
#include "MidiControlNode.h"
#include "types.h"
#include <string>
using std::string;

namespace Ingen {

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
	MidiLearnResponseEvent(Engine& engine, const string& port_path, SampleCount timestamp)
	: Event(engine, SharedPtr<Shared::Responder>(), timestamp),
	  _port_path(port_path),
	  _value(0.0f)
	{}
	
	void set_value(Sample val) { _value = val; }
	void post_process();
	
private:
	string _port_path;
	Sample _value;
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
	MidiLearnEvent(Engine& engine, SharedPtr<Shared::Responder> responder, SampleCount timestamp, const string& node_path);
	
	void pre_process();
	void execute(SampleCount nframes, FrameTime start, FrameTime end);
	void post_process();

private:
	string  _node_path;
	Node*   _node;
	
	/// Event to respond with when learned
	MidiLearnResponseEvent* _response_event;
};


} // namespace Ingen

#endif // MIDILEARNEVENT_H
