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

#ifndef CONNECTIONEVENT_H
#define CONNECTIONEVENT_H

#include <string>
#include "QueuedEvent.hpp"
#include <raul/Path.hpp>
#include "types.hpp"
using std::string;

namespace Raul {
	template <typename T> class ListNode;
	template <typename T> class Array;
}

namespace Ingen {
	
class Patch;
class Node;
class Connection;
class MidiMessage;
class Port;
class Connection;
class InputPort;
class OutputPort;
class CompiledPatch;


/** Make a Connection between two Ports.
 *
 * \ingroup engine
 */
class ConnectionEvent : public QueuedEvent
{
public:
	ConnectionEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& src_port_path, const string& dst_port_path);

	void pre_process();
	void execute(SampleCount nframes, FrameTime start, FrameTime end);
	void post_process();

private:
	
	enum ErrorType { 
		NO_ERROR,
		PARENT_PATCH_DIFFERENT,
		PORT_NOT_FOUND,
		TYPE_MISMATCH,
		DIRECTION_MISMATCH,
		ALREADY_CONNECTED,
		PARENTS_NOT_FOUND
	};
	
	Raul::Path _src_port_path;
	Raul::Path _dst_port_path;
	
	Patch*      _patch;
	Port*       _src_port;
	Port*       _dst_port;
	OutputPort* _src_output_port;
	InputPort*  _dst_input_port;

	CompiledPatch* _compiled_patch; ///< New process order for Patch
	
	Connection*                  _connection;
	Raul::ListNode<Connection*>* _patch_listnode;
	Raul::ListNode<Connection*>* _port_listnode;

	ErrorType _error;
};


} // namespace Ingen

#endif // CONNECTIONEVENT_H
