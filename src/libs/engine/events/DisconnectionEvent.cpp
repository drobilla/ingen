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

#include "DisconnectionEvent.hpp"
#include <string>
#include <raul/Maid.hpp>
#include <raul/Path.hpp>
#include "interface/Responder.hpp"
#include "Engine.hpp"
#include "Connection.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"
#include "Patch.hpp"
#include "ClientBroadcaster.hpp"
#include "Port.hpp"
#include "ObjectStore.hpp"

using std::string;
namespace Ingen {


//// DisconnectionEvent ////


DisconnectionEvent::DisconnectionEvent(Engine& engine, SharedPtr<Shared::Responder> responder, SampleCount timestamp, const string& src_port_path, const string& dst_port_path)
: QueuedEvent(engine, responder, timestamp),
  _src_port_path(src_port_path),
  _dst_port_path(dst_port_path),
  _patch(NULL),
  _src_port(NULL),
  _dst_port(NULL),
  _lookup(true),
  _process_order(NULL),
  _error(NO_ERROR)
{
}


DisconnectionEvent::DisconnectionEvent(Engine& engine, SharedPtr<Shared::Responder> responder, SampleCount timestamp, Port* const src_port, Port* const dst_port)
: QueuedEvent(engine, responder, timestamp),
  _src_port_path(src_port->path()),
  _dst_port_path(dst_port->path()),
  _patch(src_port->parent_node()->parent_patch()),
  _src_port(src_port),
  _dst_port(dst_port),
  _lookup(false),
  _process_order(NULL),
  _error(NO_ERROR)
{
	// FIXME: These break for patch ports.. is that ok?
	/*assert(src_port->is_output());
	assert(dst_port->is_input());
	assert(src_port->type() == dst_port->type());
	assert(src_port->parent_node()->parent_patch()
			== dst_port->parent_node()->parent_patch()); */
}


void
DisconnectionEvent::pre_process()
{
	if (_lookup) {
		if (_src_port_path.parent().parent() != _dst_port_path.parent().parent()
				&& _src_port_path.parent() != _dst_port_path.parent().parent()
				&& _src_port_path.parent().parent() != _dst_port_path.parent()) {
			_error = PARENT_PATCH_DIFFERENT;
			QueuedEvent::pre_process();
			return;
		}

		_src_port = _engine.object_store()->find_port(_src_port_path);
		_dst_port = _engine.object_store()->find_port(_dst_port_path);
	}
	
	if (_src_port == NULL || _dst_port == NULL) {
		_error = PORT_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}

	_dst_input_port = dynamic_cast<InputPort*>(_dst_port);
	_src_output_port = dynamic_cast<OutputPort*>(_src_port);
	assert(_src_output_port);
	assert(_dst_input_port);

	if (!_dst_input_port->is_connected_to(_src_output_port)) {
		_error = NOT_CONNECTED;
		QueuedEvent::pre_process();
		return;
	}
	
	Node* const src_node = _src_port->parent_node();
	Node* const dst_node = _dst_port->parent_node();

	// Connection to a patch port from inside the patch
	if (src_node->parent_patch() != dst_node->parent_patch()) {

		assert(src_node->parent() == dst_node || dst_node->parent() == src_node);
		if (src_node->parent() == dst_node)
			_patch = dynamic_cast<Patch*>(dst_node);
		else
			_patch = dynamic_cast<Patch*>(src_node);
	
	// Connection from a patch input to a patch output (pass through)
	} else if (src_node == dst_node && dynamic_cast<Patch*>(src_node)) {
		_patch = dynamic_cast<Patch*>(src_node);
	
	// Normal connection between nodes with the same parent
	} else {
		_patch = src_node->parent_patch();
	}

	assert(_patch);

	if (src_node == NULL || dst_node == NULL) {
		_error = PARENTS_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}
	
	for (Raul::List<Node*>::iterator i = dst_node->providers()->begin(); i != dst_node->providers()->end(); ++i)
		if ((*i) == src_node) {
			delete dst_node->providers()->erase(i);
			break;
		}

	for (Raul::List<Node*>::iterator i = src_node->dependants()->begin(); i != src_node->dependants()->end(); ++i)
		if ((*i) == dst_node) {
			delete src_node->dependants()->erase(i);
			break;
		}
	
	if (_patch->enabled())
		_process_order = _patch->build_process_order();
	
	QueuedEvent::pre_process();
}


void
DisconnectionEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);

	if (_error == NO_ERROR) {
		Raul::ListNode<Connection*>* const port_connection
			= _dst_input_port->remove_connection(_src_output_port);
		
		if (port_connection != NULL) {
			Raul::ListNode<Connection*>* const patch_connection
				= _patch->remove_connection(_src_port, _dst_port);
			
			assert(patch_connection);
			assert((Connection*)port_connection->elem() == patch_connection->elem());
			
			// Clean up both the list node and the connection itself...
			_engine.maid()->push(port_connection);
			_engine.maid()->push(patch_connection);
			_engine.maid()->push(port_connection->elem());
	
			if (_patch->process_order() != NULL)
				_engine.maid()->push(_patch->process_order());
			_patch->process_order(_process_order);
		} else {
			_error = CONNECTION_NOT_FOUND;
		}
	}
}


void
DisconnectionEvent::post_process()
{
	if (_error == NO_ERROR) {
		_responder->respond_ok();
		_engine.broadcaster()->send_disconnection(_src_port->path(), _dst_port->path());
	} else {
		// FIXME: better error messages
		string msg = "Unable to disconnect ";
		msg.append(_src_port_path + " -> " + _dst_port_path);
		cerr << "DISCONNECTION ERROR " << (unsigned)_error << endl;
		_responder->respond_error(msg);
	}
}


} // namespace Ingen

