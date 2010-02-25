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

#include <string>
#include <boost/format.hpp>
#include "raul/Maid.hpp"
#include "raul/Path.hpp"
#include "ClientBroadcaster.hpp"
#include "Connect.hpp"
#include "ConnectionImpl.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"
#include "PatchImpl.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
#include "Request.hpp"
#include "types.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Events {

using namespace Shared;


Connect::Connect(Engine& engine, SharedPtr<Request> request, SampleCount timestamp, const Path& src_port_path, const Path& dst_port_path)
	: QueuedEvent(engine, request, timestamp)
	, _src_port_path(src_port_path)
	, _dst_port_path(dst_port_path)
	, _patch(NULL)
	, _src_output_port(NULL)
	, _dst_input_port(NULL)
	, _compiled_patch(NULL)
	, _port_listnode(NULL)
	, _buffers(NULL)
{
}


void
Connect::pre_process()
{
	PortImpl* src_port = _engine.engine_store()->find_port(_src_port_path);
	PortImpl* dst_port = _engine.engine_store()->find_port(_dst_port_path);
	if (!src_port || !dst_port) {
		_error = PORT_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}

	_dst_input_port = dynamic_cast<InputPort*>(dst_port);
	_src_output_port = dynamic_cast<OutputPort*>(src_port);
	if (!_dst_input_port || !_src_output_port) {
		_error = DIRECTION_MISMATCH;
		QueuedEvent::pre_process();
		return;
	}

	NodeImpl* const src_node = src_port->parent_node();
	NodeImpl* const dst_node = dst_port->parent_node();
	if (!src_node || !dst_node) {
		_error = PARENTS_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}

	if (src_node->parent() != dst_node->parent()
			&& src_node != dst_node->parent()
			&& src_node->parent() != dst_node) {
		_error = PARENT_PATCH_DIFFERENT;
		QueuedEvent::pre_process();
		return;
	}

	if (!ConnectionImpl::can_connect(_src_output_port, _dst_input_port)) {
		_error = TYPE_MISMATCH;
		QueuedEvent::pre_process();
		return;
	}

	// Connection to a patch port from inside the patch
	if (src_node->parent_patch() != dst_node->parent_patch()) {
		assert(src_node->parent() == dst_node || dst_node->parent() == src_node);
		if (src_node->parent() == dst_node)
			_patch = dynamic_cast<PatchImpl*>(dst_node);
		else
			_patch = dynamic_cast<PatchImpl*>(src_node);

	// Connection from a patch input to a patch output (pass through)
	} else if (src_node == dst_node && dynamic_cast<PatchImpl*>(src_node)) {
		_patch = dynamic_cast<PatchImpl*>(src_node);

	// Normal connection between nodes with the same parent
	} else {
		_patch = src_node->parent_patch();
	}

	if (_patch->has_connection(_src_output_port, _dst_input_port)) {
		_error = ALREADY_CONNECTED;
		QueuedEvent::pre_process();
		return;
	}

	_connection = SharedPtr<ConnectionImpl>(
			new ConnectionImpl(*_engine.buffer_factory(), _src_output_port, _dst_input_port));

	_port_listnode = new InputPort::Connections::Node(_connection);

	// Need to be careful about patch port connections here and adding a node's
	// parent as a dependant/provider, or adding a patch as it's own provider...
	if (src_node != dst_node && src_node->parent() == dst_node->parent()) {
		dst_node->providers()->push_back(new Raul::List<NodeImpl*>::Node(src_node));
		src_node->dependants()->push_back(new Raul::List<NodeImpl*>::Node(dst_node));
	}

	_patch->add_connection(_connection);
	_dst_input_port->increment_num_connections();

	switch (_dst_input_port->num_connections()) {
	case 1:
		_connection->allocate_buffer(*_engine.buffer_factory());
		break;
	case 2:
		_buffers = new Raul::Array<BufferFactory::Ref>(_dst_input_port->poly());
		_dst_input_port->get_buffers(*_engine.buffer_factory(), _buffers, _dst_input_port->poly());
	default:
		break;
	}

	if (_patch->enabled())
		_compiled_patch = _patch->compile();

	QueuedEvent::pre_process();
}


void
Connect::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_error == NO_ERROR) {
		// This must be inserted here, since they're actually used by the audio thread
		_dst_input_port->add_connection(_port_listnode);
		if (_buffers)
			_engine.maid()->push(_dst_input_port->set_buffers(_buffers));
		else
			_dst_input_port->setup_buffers(*_engine.buffer_factory(), _dst_input_port->poly());
		_dst_input_port->connect_buffers();
		_engine.maid()->push(_patch->compiled_patch());
		_patch->compiled_patch(_compiled_patch);
	}
}


void
Connect::post_process()
{
	std::ostringstream ss;
	if (_error == NO_ERROR) {
		_request->respond_ok();
		_engine.broadcaster()->connect(_src_port_path, _dst_port_path);
		return;
	}

	ss << boost::format("Unable to make connection %1% -> %2% (") % _src_port_path % _dst_port_path;

	switch (_error) {
	case PARENT_PATCH_DIFFERENT:
		ss << "Ports have mismatched parents"; break;
	case PORT_NOT_FOUND:
		ss << "Port not found"; break;
	case TYPE_MISMATCH:
		ss << "Type mismatch"; break;
	case DIRECTION_MISMATCH:
		ss << "Direction mismatch"; break;
	case ALREADY_CONNECTED:
		ss << "Already connected"; break;
	case PARENTS_NOT_FOUND:
		ss << "Parents not found"; break;
	default:
		ss << "Unknown error";
	}
	ss << ")";
	_request->respond_error(ss.str());
}


} // namespace Ingen
} // namespace Events

