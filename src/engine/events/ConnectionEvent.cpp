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


#include <string>
#include <boost/format.hpp>
#include <raul/Maid.hpp>
#include <raul/Path.hpp>
#include "ClientBroadcaster.hpp"
#include "ConnectionEvent.hpp"
#include "ConnectionImpl.hpp"
#include "Engine.hpp"
#include "InputPort.hpp"
#include "EngineStore.hpp"
#include "OutputPort.hpp"
#include "PatchImpl.hpp"
#include "PortImpl.hpp"
#include "Responder.hpp"
#include "types.hpp"

using std::string;
namespace Ingen {


ConnectionEvent::ConnectionEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& src_port_path, const string& dst_port_path)
: QueuedEvent(engine, responder, timestamp),
  _src_port_path(src_port_path),
  _dst_port_path(dst_port_path),
  _patch(NULL),
  _src_port(NULL),
  _dst_port(NULL),
  _compiled_patch(NULL),
  _patch_listnode(NULL),
  _port_listnode(NULL),
  _error(NO_ERROR)
{
}


void
ConnectionEvent::pre_process()
{
	if (_src_port_path.parent().parent() != _dst_port_path.parent().parent()
			&& _src_port_path.parent() != _dst_port_path.parent().parent()
			&& _src_port_path.parent().parent() != _dst_port_path.parent()) {
		_error = PARENT_PATCH_DIFFERENT;
		QueuedEvent::pre_process();
		return;
	}
	
	_src_port = _engine.engine_store()->find_port(_src_port_path);
	_dst_port = _engine.engine_store()->find_port(_dst_port_path);
	
	if (_src_port == NULL || _dst_port == NULL) {
		_error = PORT_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}

	if ( ! (_src_port->type() == _dst_port->type()
			|| ( (_src_port->type() == DataType::CONTROL || _src_port->type() == DataType::AUDIO)
				&& (_dst_port->type() == DataType::CONTROL || _dst_port->type() == DataType::AUDIO) ))) {
		_error = TYPE_MISMATCH;
		QueuedEvent::pre_process();
		return;
	}
			
	_dst_input_port = dynamic_cast<InputPort*>(_dst_port);
	_src_output_port = dynamic_cast<OutputPort*>(_src_port);
	
	if (!_dst_input_port || !_src_output_port) {
		_error = DIRECTION_MISMATCH;
		QueuedEvent::pre_process();
		return;
	}

	NodeImpl* const src_node = _src_port->parent_node();
	NodeImpl* const dst_node = _dst_port->parent_node();

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

	assert(_patch);
	
	if (_patch->has_connection(_src_output_port, _dst_input_port)) {
		_error = ALREADY_CONNECTED;
		QueuedEvent::pre_process();
		return;
	}

	if (src_node == NULL || dst_node == NULL) {
		_error = PARENTS_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}
	
	if (_patch != src_node && src_node->parent() != _patch && dst_node->parent() != _patch) {
		_error = PARENTS_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}

	_connection = SharedPtr<ConnectionImpl>(new ConnectionImpl(_src_port, _dst_port));
	_patch_listnode = new PatchImpl::Connections::Node(_connection);
	_port_listnode = new InputPort::Connections::Node(_connection);
	
	// Need to be careful about patch port connections here and adding a node's
	// parent as a dependant/provider, or adding a patch as it's own provider...
	if (src_node != dst_node && src_node->parent() == dst_node->parent()) {
		dst_node->providers()->push_back(new Raul::List<NodeImpl*>::Node(src_node));
		src_node->dependants()->push_back(new Raul::List<NodeImpl*>::Node(dst_node));
	}
		
	_patch->add_connection(_patch_listnode);

	if (_patch->enabled())
		_compiled_patch = _patch->compile();

	QueuedEvent::pre_process();
}


void
ConnectionEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_error == NO_ERROR) {
		// This must be inserted here, since they're actually used by the audio thread
		_dst_input_port->add_connection(_port_listnode);
		if (_patch->compiled_patch() != NULL)
			_engine.maid()->push(_patch->compiled_patch());
		_patch->compiled_patch(_compiled_patch);
	}
}


void
ConnectionEvent::post_process()
{
	std::ostringstream ss;
	if (_error == NO_ERROR) {
		_responder->respond_ok();
		_engine.broadcaster()->send_connection(_connection);
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
	_responder->respond_error(ss.str());
}


} // namespace Ingen

