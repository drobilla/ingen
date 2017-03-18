/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

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
#include "PreProcessContext.hpp"

namespace Ingen {
namespace Server {
namespace Events {

Delete::Delete(Engine&          engine,
               SPtr<Interface>  client,
               int32_t          id,
               FrameTime        time,
               const Raul::URI& uri)
	: Event(engine, client, id, time)
	, _uri(uri)
	, _engine_port(NULL)
	, _disconnect_event(NULL)
{
	if (uri_is_path(uri)) {
		_path = uri_to_path(uri);
	}
}

Delete::~Delete()
{
	delete _disconnect_event;
	for (ControlBindings::Binding* b : _removed_bindings) {
		delete b;
	}
}

bool
Delete::pre_process(PreProcessContext& ctx)
{
	if (_path.is_root() || _path == "/control" || _path == "/notify") {
		return Event::pre_process_done(Status::NOT_DELETABLE, _path);
	}

	_engine.control_bindings()->get_all(_path, _removed_bindings);

	Store::iterator iter = _engine.store()->find(_path);
	if (iter == _engine.store()->end()) {
		return Event::pre_process_done(Status::NOT_FOUND, _path);
	}

	if (!(_block = dynamic_ptr_cast<BlockImpl>(iter->second))) {
		_port = dynamic_ptr_cast<DuplexPort>(iter->second);
	}

	if ((!_block && !_port) || (_port && !_engine.driver()->dynamic_ports())) {
		return Event::pre_process_done(Status::NOT_DELETABLE, _path);
	}

	GraphImpl* parent = _block ? _block->parent_graph() : _port->parent_graph();
	if (!parent) {
		return Event::pre_process_done(Status::INTERNAL_ERROR, _path);
	}

	// Take a writer lock while we modify the store
	std::lock_guard<Store::Mutex> lock(_engine.store()->mutex());

	_engine.store()->remove(iter, _removed_objects);

	if (_block) {
		parent->remove_block(*_block);
		_disconnect_event = new DisconnectAll(_engine, parent, _block.get());
		_disconnect_event->pre_process(ctx);
		_compiled_graph = ctx.maybe_compile(*_engine.maid(), *parent);
	} else if (_port) {
		parent->remove_port(*_port);
		_disconnect_event = new DisconnectAll(_engine, parent, _port.get());
		_disconnect_event->pre_process(ctx);

		_compiled_graph = ctx.maybe_compile(*_engine.maid(), *parent);
		if (parent->enabled()) {
			_ports_array = parent->build_ports_array(*_engine.maid());
			assert(_ports_array->size() == parent->num_ports_non_rt());
		}

		if (!parent->parent()) {
			_engine_port = _engine.driver()->get_port(_port->path());
		}
	}

	return Event::pre_process_done(Status::SUCCESS);
}

void
Delete::execute(RunContext& context)
{
	if (_status != Status::SUCCESS) {
		return;
	}

	if (_disconnect_event) {
		_disconnect_event->execute(context);
	}

	if (!_removed_bindings.empty()) {
		_engine.control_bindings()->remove(context, _removed_bindings);
	}

	GraphImpl* parent = _block ? _block->parent_graph() : NULL;
	if (_port) {
		parent = _port->parent_graph();
		parent->set_external_ports(std::move(_ports_array));

		if (_engine_port) {
			_engine.driver()->remove_port(context, _engine_port);
		}
	}

	if (parent && _compiled_graph) {
		parent->set_compiled_graph(std::move(_compiled_graph));
	}
}

void
Delete::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS && (_block || _port)) {
		if (_block) {
			_block->deactivate();
		}

		_engine.broadcaster()->del(_uri);
	}

	if (_engine_port) {
		_engine.driver()->unregister_port(*_engine_port);
		delete _engine_port;
	}
}

void
Delete::undo(Interface& target)
{
	auto i = _removed_objects.find(_path);
	if (i != _removed_objects.end()) {
		target.put(_uri, i->second->properties());
		if (_disconnect_event) {
			_disconnect_event->undo(target);
		}
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
