/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "raul/Maid.hpp"
#include "raul/Path.hpp"

#include "Broadcaster.hpp"
#include "ControlBindings.hpp"
#include "Delete.hpp"
#include "DisconnectAll.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EnginePort.hpp"
#include "EngineStore.hpp"
#include "NodeImpl.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {
namespace Events {

Delete::Delete(Engine&          engine,
               Interface*       client,
               int32_t          id,
               FrameTime        time,
               const Raul::URI& uri)
	: Event(engine, client, id, time)
	, _uri(uri)
	, _store_iterator(engine.engine_store()->end())
	, _garbage(NULL)
	, _engine_port(NULL)
	, _patch_node_listnode(NULL)
	, _patch_port_listnode(NULL)
	, _ports_array(NULL)
	, _compiled_patch(NULL)
	, _disconnect_event(NULL)
	, _lock(engine.engine_store()->lock(), Glib::NOT_LOCK)
{
	if (Raul::Path::is_path(uri))
		_path = Raul::Path(uri.str());
}

Delete::~Delete()
{
	delete _disconnect_event;
}

void
Delete::pre_process()
{
	if (_path.is_root() || _path == "path:/control_in" || _path == "path:/control_out") {
		Event::pre_process();
		return;
	}

	_lock.acquire();

	_removed_bindings = _engine.control_bindings()->remove(_path);

	_store_iterator = _engine.engine_store()->find(_path);

	if (_store_iterator != _engine.engine_store()->end())  {
		_node = PtrCast<NodeImpl>(_store_iterator->second);

		if (!_node)
			_port = PtrCast<PortImpl>(_store_iterator->second);
	}

	if (_store_iterator != _engine.engine_store()->end()) {
		_removed_table = _engine.engine_store()->remove(_store_iterator);
	}

	if (_node && !_path.is_root()) {
		assert(_node->parent_patch());
		_patch_node_listnode = _node->parent_patch()->remove_node(_path.symbol());
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
		_patch_port_listnode = _port->parent_patch()->remove_port(_path.symbol());
		if (_patch_port_listnode) {
			assert(_patch_port_listnode->elem() == _port.get());

			_disconnect_event = new DisconnectAll(_engine, _port->parent_patch(), _port.get());
			_disconnect_event->pre_process();

			if (_port->parent_patch()->enabled()) {
				// FIXME: is this called multiple times?
				_compiled_patch = _port->parent_patch()->compile();
				_ports_array    = _port->parent_patch()->build_ports_array();
				assert(_ports_array->size() == _port->parent_patch()->num_ports_non_rt());
			}
		}

	}

	Event::pre_process();
}

void
Delete::execute(ProcessContext& context)
{
	Event::execute(context);

	PatchImpl* parent_patch = NULL;

	if (_patch_node_listnode) {
		assert(_node);

		if (_disconnect_event)
			_disconnect_event->execute(context);

		parent_patch = _node->parent_patch();

	} else if (_patch_port_listnode) {
		assert(_port);

		if (_disconnect_event)
			_disconnect_event->execute(context);

		parent_patch = _port->parent_patch();

		_engine.maid()->push(_port->parent_patch()->external_ports());
		_port->parent_patch()->external_ports(_ports_array);

		if ( ! _port->parent_patch()->parent())
			_garbage = _engine.driver()->remove_port(context, _port->path(), &_engine_port);
	}

	if (parent_patch) {
		_engine.maid()->push(parent_patch->compiled_patch());
		parent_patch->compiled_patch(_compiled_patch);
	}
}

void
Delete::post_process()
{
	_lock.release();
	_removed_bindings.reset();

	if (!Raul::Path::is_path(_uri)
	    || _path.is_root() || _path == "path:/control_in" || _path == "path:/control_out") {
		// XXX: Report error?  Silently ignore?
	} else if (!_node && !_port) {
		respond(NOT_FOUND);
	} else if (_patch_node_listnode || _patch_port_listnode) {
		if (_patch_node_listnode) {
			_node->deactivate();
			delete _patch_node_listnode;
		} else if (_patch_port_listnode) {
			delete _patch_port_listnode;
		}
		
		respond(SUCCESS);
		_engine.broadcaster()->bundle_begin();
		if (_disconnect_event) {
			_disconnect_event->post_process();
		}
		_engine.broadcaster()->del(_path);
		_engine.broadcaster()->bundle_end();
	} else {
		respond(FAILURE);
	}

	if (_engine_port) {
		_engine_port->destroy();
	}

	delete _garbage;
}

} // namespace Events
} // namespace Server
} // namespace Ingen
