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

#include "ingen/Store.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"

#include "BlockImpl.hpp"
#include "Broadcaster.hpp"
#include "ControlBindings.hpp"
#include "Delete.hpp"
#include "DisconnectAll.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EnginePort.hpp"
#include "GraphImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {
namespace Events {

Delete::Delete(Engine&              engine,
               SharedPtr<Interface> client,
               int32_t              id,
               FrameTime            time,
               const Raul::URI&     uri)
	: Event(engine, client, id, time)
	, _uri(uri)
	, _engine_port(NULL)
	, _ports_array(NULL)
	, _compiled_graph(NULL)
	, _disconnect_event(NULL)
	, _lock(engine.store()->lock(), Glib::NOT_LOCK)
{
	if (Node::uri_is_path(uri)) {
		_path = Node::uri_to_path(uri);
	}
}

Delete::~Delete()
{
	delete _disconnect_event;
}

bool
Delete::pre_process()
{
	if (_path.is_root() || _path == "/control_in" || _path == "/control_out") {
		return Event::pre_process_done(Status::NOT_DELETABLE, _path);
	}

	_lock.acquire();

	_removed_bindings = _engine.control_bindings()->remove(_path);

	Store::iterator iter = _engine.store()->find(_path);
	if (iter == _engine.store()->end()) {
		return Event::pre_process_done(Status::NOT_FOUND, _path);
	}

	if (!(_block = PtrCast<BlockImpl>(iter->second))) {
		_port = PtrCast<DuplexPort>(iter->second);
	}

	if (!_block && !_port) {
		return Event::pre_process_done(Status::NOT_DELETABLE, _path);
	}

	GraphImpl* parent = _block ? _block->parent_graph() : _port->parent_graph();
	if (!parent) {
		return Event::pre_process_done(Status::INTERNAL_ERROR, _path);
	}

	_engine.store()->remove(iter, _removed_objects);

	if (_block) {
		parent->remove_block(*_block);
		_disconnect_event = new DisconnectAll(_engine, parent, _block.get());
		_disconnect_event->pre_process();

		if (parent->enabled()) {
			_compiled_graph = parent->compile();
		}
	} else if (_port) {
		parent->remove_port(*_port);
		_disconnect_event = new DisconnectAll(_engine, parent, _port.get());
		_disconnect_event->pre_process();

		if (parent->enabled()) {
			_compiled_graph = parent->compile();
			_ports_array    = parent->build_ports_array();
			assert(_ports_array->size() == parent->num_ports_non_rt());
		}

		if (!parent->parent()) {
			_engine_port = _engine.driver()->get_port(_port->path());
		}
	}

	return Event::pre_process_done(Status::SUCCESS);
}

void
Delete::execute(ProcessContext& context)
{
	if (_disconnect_event) {
		_disconnect_event->execute(context);
	}

	GraphImpl* parent = _block ? _block->parent_graph() : NULL;
	if (_port) {
		parent = _port->parent_graph();
		_engine.maid()->dispose(parent->external_ports());
		parent->external_ports(_ports_array);

		if (_engine_port) {
			_engine.driver()->remove_port(context, _engine_port);
		}
	}

	if (parent) {
		parent->set_compiled_graph(_compiled_graph);
	}
}

void
Delete::post_process()
{
	_lock.release();
	_removed_bindings.reset();

	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS && (_block || _port)) {
		if (_block) {
			_block->deactivate();
		}

		if (_disconnect_event) {
			_disconnect_event->post_process();
		}
		_engine.broadcaster()->del(_uri);
	}

	if (_engine_port) {
		_engine.driver()->unregister_port(*_engine_port);
		delete _engine_port;
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
