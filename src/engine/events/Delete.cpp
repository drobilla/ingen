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

#include "raul/Maid.hpp"
#include "raul/Path.hpp"
#include "Delete.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "PatchImpl.hpp"
#include "NodeBase.hpp"
#include "PluginImpl.hpp"
#include "Driver.hpp"
#include "DisconnectAll.hpp"
#include "ClientBroadcaster.hpp"
#include "EngineStore.hpp"
#include "EventSource.hpp"
#include "PortImpl.hpp"

using namespace std;

namespace Ingen {
namespace Events {

using namespace Shared;


Delete::Delete(Engine& engine, SharedPtr<Responder> responder, FrameTime time, EventSource* source, const Raul::Path& path)
	: QueuedEvent(engine, responder, time, true, source)
	, _path(path)
	, _store_iterator(engine.engine_store()->end())
	, _driver_port(NULL)
	, _patch_node_listnode(NULL)
	, _patch_port_listnode(NULL)
	, _ports_array(NULL)
	, _compiled_patch(NULL)
	, _disconnect_event(NULL)
{
	assert(_source);
}


Delete::~Delete()
{
	delete _disconnect_event;
}


void
Delete::pre_process()
{
	_store_iterator = _engine.engine_store()->find(_path);

	if (_store_iterator != _engine.engine_store()->end())  {
		_node = PtrCast<NodeImpl>(_store_iterator->second);

		if (!_node)
			_port = PtrCast<PortImpl>(_store_iterator->second);
	}

	if (_store_iterator != _engine.engine_store()->end()) {
		_removed_table = _engine.engine_store()->remove(_store_iterator);
	}

	if (_node != NULL && !_path.is_root()) {
		assert(_node->parent_patch());
		_patch_node_listnode = _node->parent_patch()->remove_node(_path.name());
		if (_patch_node_listnode) {
			assert(_patch_node_listnode->elem() == _node.get());

			_disconnect_event = new DisconnectAll(_engine, _node->parent_patch(), _node.get());
			_disconnect_event->pre_process();

			if (_node->parent_patch()->enabled()) {
				// FIXME: is this called multiple times?
				_compiled_patch = _node->parent_patch()->compile();
#ifndef NDEBUG
				// Be sure node is removed from process order, so it can be deleted
				for (size_t i=0; i < _compiled_patch->size(); ++i) {
					assert(_compiled_patch->at(i).node() != _node.get());
					// FIXME: check providers/dependants too
				}
#endif
			}
		}
	} else if (_port) {
		assert(_port->parent_patch());
		_patch_port_listnode = _port->parent_patch()->remove_port(_path.name());
		if (_patch_port_listnode) {
			assert(_patch_port_listnode->elem() == _port.get());

			_disconnect_event = new DisconnectAll(_engine, _port->parent_patch(), _port.get());
			_disconnect_event->pre_process();

			if (_port->parent_patch()->enabled()) {
				// FIXME: is this called multiple times?
				_compiled_patch = _port->parent_patch()->compile();
				_ports_array   = _port->parent_patch()->build_ports_array();
				assert(_ports_array->size() == _port->parent_patch()->num_ports());
			}
		}

	}

	QueuedEvent::pre_process();
}


void
Delete::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_patch_node_listnode) {
		assert(_node);

		if (_disconnect_event)
			_disconnect_event->execute(context);

		if (_node->parent_patch()->compiled_patch())
			_engine.maid()->push(_node->parent_patch()->compiled_patch());

		_node->parent_patch()->compiled_patch(_compiled_patch);

	} else if (_patch_port_listnode) {
		assert(_port);

		if (_disconnect_event)
			_disconnect_event->execute(context);

		if (_port->parent_patch()->compiled_patch())
			_engine.maid()->push(_port->parent_patch()->compiled_patch());

		_port->parent_patch()->compiled_patch(_compiled_patch);

		if (_port->parent_patch()->external_ports())
			_engine.maid()->push(_port->parent_patch()->external_ports());

		_port->parent_patch()->external_ports(_ports_array);

		if ( ! _port->parent_patch()->parent()) {
			_driver_port = _engine.driver()->remove_port(_port->path());

			// Apparently this needs to be called in post_process??
			//if (_driver_port)
			//	_driver_port->elem()->unregister();
		}
	}

	if (_source)
		_source->unblock();
}


void
Delete::post_process()
{
	if (!_node && !_port) {
		if (_path.is_root()) {
			_responder->respond_error("You can not destroy the root patch (/)");
		} else {
			string msg = string("Could not find object ") + _path.str() + " to destroy";
			_responder->respond_error(msg);
		}
	}

	if (_patch_node_listnode) {
		assert(_node);
		_node->deactivate();
		_responder->respond_ok();
		_engine.broadcaster()->bundle_begin();
		if (_disconnect_event)
			_disconnect_event->post_process();
		_engine.broadcaster()->send_deleted(_path);
		_engine.broadcaster()->bundle_end();
		_engine.maid()->push(_patch_node_listnode);
	} else if (_patch_port_listnode) {
		assert(_port);
		_responder->respond_ok();
		_engine.broadcaster()->bundle_begin();
		if (_disconnect_event)
			_disconnect_event->post_process();
		_engine.broadcaster()->send_deleted(_path);
		_engine.broadcaster()->bundle_end();
		_engine.maid()->push(_patch_port_listnode);
	} else {
		_responder->respond_error("Unable to destroy object");
	}

	if (_driver_port) {
		_driver_port->elem()->destroy();
		_engine.maid()->push(_driver_port);
	}
}


} // namespace Ingen
} // namespace Events
