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

#include "events/DisconnectAll.hpp"

#include "ArcImpl.hpp"
#include "BlockImpl.hpp"
#include "Broadcaster.hpp"
#include "CompiledGraph.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "InputPort.hpp"
#include "NodeImpl.hpp"
#include "PortImpl.hpp"
#include "PreProcessContext.hpp"
#include "events/Disconnect.hpp"

#include "ingen/Interface.hpp"
#include "ingen/Node.hpp"
#include "ingen/Status.hpp"
#include "ingen/Store.hpp"
#include "raul/Maid.hpp"

#include <memory>
#include <mutex>
#include <set>
#include <utility>

namespace ingen {
namespace server {
namespace events {

DisconnectAll::DisconnectAll(Engine&                           engine,
                             const std::shared_ptr<Interface>& client,
                             SampleCount                       timestamp,
                             const ingen::DisconnectAll&       msg)
    : Event(engine, client, msg.seq, timestamp)
    , _msg(msg)
    , _parent(nullptr)
    , _block(nullptr)
    , _port(nullptr)
    , _deleting(false)
{
}

/** Internal version for use by other events.
 */
DisconnectAll::DisconnectAll(Engine&    engine,
                             GraphImpl* parent,
                             Node*      object)
	: Event(engine)
	, _msg{0, parent->path(), object->path()}
	, _parent(parent)
	, _block(dynamic_cast<BlockImpl*>(object))
	, _port(dynamic_cast<PortImpl*>(object))
	, _deleting(true)
{
}

DisconnectAll::~DisconnectAll()
{
	for (auto& i : _impls) {
		delete i;
	}
}

bool
DisconnectAll::pre_process(PreProcessContext& ctx)
{
	std::unique_lock<Store::Mutex> lock(_engine.store()->mutex(),
	                                    std::defer_lock);

	if (!_deleting) {
		lock.lock();

		_parent = dynamic_cast<GraphImpl*>(_engine.store()->get(_msg.graph));
		if (!_parent) {
			return Event::pre_process_done(Status::PARENT_NOT_FOUND,
			                               _msg.graph);
		}

		NodeImpl* const object = dynamic_cast<NodeImpl*>(
			_engine.store()->get(_msg.path));
		if (!object) {
			return Event::pre_process_done(Status::NOT_FOUND, _msg.path);
		}

		if (object->parent_graph() != _parent
		    && object->parent()->parent_graph() != _parent) {
			return Event::pre_process_done(Status::INVALID_PARENT, _msg.graph);
		}

		// Only one of these will succeed
		_block = dynamic_cast<BlockImpl*>(object);
		_port  = dynamic_cast<PortImpl*>(object);

		if (!_block && !_port) {
			return Event::pre_process_done(Status::INTERNAL_ERROR, _msg.path);
		}
	}

	// Create disconnect events to erase adjacent arcs in parent
	for (const auto& a : adjacent_arcs(_parent)) {
		_impls.push_back(
			new Disconnect::Impl(_engine,
			                     _parent,
			                     dynamic_cast<PortImpl*>(a->tail()),
			                     dynamic_cast<InputPort*>(a->head())));
	}

	// Create disconnect events to erase adjacent arcs in parent's parent
	if (_port && _parent->parent()) {
		auto* const parent_parent = dynamic_cast<GraphImpl*>(_parent->parent());
		for (const auto& a : adjacent_arcs(parent_parent)) {
			_impls.push_back(
				new Disconnect::Impl(_engine,
				                     parent_parent,
				                     dynamic_cast<PortImpl*>(a->tail()),
				                     dynamic_cast<InputPort*>(a->head())));
		}
	}

	if (!_deleting && ctx.must_compile(*_parent)) {
		if (!(_compiled_graph = compile(*_engine.maid(), *_parent))) {
			return Event::pre_process_done(Status::COMPILATION_FAILED);
		}
	}

	return Event::pre_process_done(Status::SUCCESS);
}

void
DisconnectAll::execute(RunContext& ctx)
{
	if (_status == Status::SUCCESS) {
		for (auto& i : _impls) {
			i->execute(ctx,
			           !_deleting || (i->head()->parent_block() != _block));
		}
	}

	if (_compiled_graph) {
		_parent->set_compiled_graph(std::move(_compiled_graph));
	}
}

void
DisconnectAll::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS) {
		_engine.broadcaster()->message(_msg);
	}
}

void
DisconnectAll::undo(Interface& target)
{
	for (auto& i : _impls) {
		target.connect(i->tail()->path(), i->head()->path());
	}
}

std::set<ArcImpl*>
DisconnectAll::adjacent_arcs(GraphImpl* const graph)
{
	std::set<ArcImpl*> arcs;
	for (const auto& a : graph->arcs()) {
		auto* const arc = static_cast<ArcImpl*>(a.second.get());
		if (_block) {
			if (arc->tail()->parent_block() == _block
			    || arc->head()->parent_block() == _block) {
				arcs.insert(arc);
			}
		} else if (_port) {
			if (arc->tail() == _port || arc->head() == _port) {
				arcs.insert(arc);
			}
		}
	}

	return arcs;
}

} // namespace events
} // namespace server
} // namespace ingen
