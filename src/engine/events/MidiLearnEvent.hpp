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

#include "QueuedEvent.hpp"
#include "InternalController.hpp"
#include "types.hpp"
#include <string>

namespace Ingen {

class NodeImpl;
class ControlChangeEvent;


/** A MIDI learn event (used by control node to learn controller number).
 *
 * \ingroup engine
 */
class MidiLearnEvent : public QueuedEvent
{
public:
	MidiLearnEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const std::string& node_path);
	
	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	enum ErrorType { 
		NO_ERROR,
		INVALID_NODE_TYPE
	};

	ErrorType         _error;
	const std::string _node_path;
	NodeImpl*         _node;
};


} // namespace Ingen

#endif // MIDILEARNEVENT_H
