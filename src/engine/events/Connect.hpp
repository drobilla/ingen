/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef INGEN_EVENTS_CONNECT_HPP
#define INGEN_EVENTS_CONNECT_HPP

#include "raul/Path.hpp"
#include "QueuedEvent.hpp"
#include "PatchImpl.hpp"
#include "InputPort.hpp"
#include "types.hpp"

namespace Raul {
	template <typename T> class ListNode;
	template <typename T> class Array;
}

namespace Ingen {

class PatchImpl;
class NodeImpl;
class ConnectionImpl;
class MidiMessage;
class PortImpl;
class InputPort;
class OutputPort;
class CompiledPatch;

namespace Events {


/** Make a Connection between two Ports.
 *
 * \ingroup engine
 */
class Connect : public QueuedEvent
{
public:
	Connect(Engine& engine, SharedPtr<Request> request, SampleCount timestamp, const Raul::Path& src_port_path, const Raul::Path& dst_port_path);

	void pre_process();
	void execute(ProcessContext& context);
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

	PatchImpl*  _patch;
	PortImpl*   _src_port;
	PortImpl*   _dst_port;
	OutputPort* _src_output_port;
	InputPort*  _dst_input_port;

	CompiledPatch* _compiled_patch; ///< New process order for Patch

	SharedPtr<ConnectionImpl>     _connection;
	PatchImpl::Connections::Node* _patch_listnode;
	InputPort::Connections::Node* _port_listnode;

	ErrorType _error;
};


} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_CONNECT_HPP
