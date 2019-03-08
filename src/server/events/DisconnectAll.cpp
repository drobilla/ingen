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
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "InputPort.hpp"
#include "PortImpl.hpp"
#include "PreProcessContext.hpp"
#include "events/Disconnect.hpp"
#include "util.hpp"

#include "ingen/Store.hpp"
#include "raul/Array.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"

#include <mutex>
#include <set>
#include <utility>

namespace ingen {
namespace server {
namespace events {

DisconnectAll::DisconnectAll(Engine&                     engine,
                             SPtr<Interface>             client,
                             SampleCount                 timestamp,
                             const ingen::DisconnectAll& msg)
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

	// Find set of arcs to remove
	std::set<ArcImpl*> to_remove;
	for (const auto& a : _parent->arcs()) {
		ArcImpl* const arc = (ArcImpl*)a.second.get();
		if (_block) {
			if (arc->tail()->parent_block() == _block
			    || arc->head()->parent_block() == _block) {
				to_remove.insert(arc);
			}
		} else if (_port) {
			if (arc->tail() == _port || arc->head() == _port) {
				to_remove.insert(arc);
			}
		}
	}

	// Create disconnect events (which erases from _parent->arcs())
	for (const auto& a : to_remove) {
		_impls.push_back(new Disconnect::Impl(
			                 _engine, _parent,
			                 dynamic_cast<PortImpl*>(a->tail()),
			                 dynamic_cast<InputPort*>(a->head())));
	}

	if (!_deleting && ctx.must_compile(*_parent)) {
		if (!(_compiled_graph = compile(*_engine.maid(), *_parent))) {
			return Event::pre_process_done(Status::COMPILATION_FAILED);
		}
	}

	return Event::pre_process_done(Status::SUCCESS);
}

void
DisconnectAll::execute(RunContext& context)
{
	if (_status == Status::SUCCESS) {
		for (auto& i : _impls) {
			i->execute(context,
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

} // namespace events
} // namespace server
} // namespace ingen
