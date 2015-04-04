/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <set>

#include <boost/format.hpp>

#include "ingen/Store.hpp"
#include "raul/Array.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"

#include "ArcImpl.hpp"
#include "BlockImpl.hpp"
#include "Broadcaster.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"
#include "PortImpl.hpp"
#include "events/Disconnect.hpp"
#include "events/DisconnectAll.hpp"
#include "util.hpp"

namespace Ingen {
namespace Server {
namespace Events {

DisconnectAll::DisconnectAll(Engine&           engine,
                             SPtr<Interface>   client,
                             int32_t           id,
                             SampleCount       timestamp,
                             const Raul::Path& parent_path,
                             const Raul::Path& path)
	: Event(engine, client, id, timestamp)
	, _parent_path(parent_path)
	, _path(path)
	, _parent(NULL)
	, _block(NULL)
	, _port(NULL)
	, _compiled_graph(NULL)
	, _deleting(false)
{
}

/** Internal version for use by other events.
 */
DisconnectAll::DisconnectAll(Engine&    engine,
                             GraphImpl* parent,
                             Node*      object)
	: Event(engine)
	, _parent_path(parent->path())
	, _path(object->path())
	, _parent(parent)
	, _block(dynamic_cast<BlockImpl*>(object))
	, _port(dynamic_cast<PortImpl*>(object))
	, _compiled_graph(NULL)
	, _deleting(true)
{
}

DisconnectAll::~DisconnectAll()
{
	for (auto& i : _impls)
		delete i;
}

bool
DisconnectAll::pre_process()
{
	std::unique_lock<std::mutex> lock(_engine.store()->mutex(), std::defer_lock);

	if (!_deleting) {
		lock.lock();

		_parent = dynamic_cast<GraphImpl*>(_engine.store()->get(_parent_path));
		if (!_parent) {
			return Event::pre_process_done(Status::PARENT_NOT_FOUND,
			                               _parent_path);
		}

		NodeImpl* const object = dynamic_cast<NodeImpl*>(
			_engine.store()->get(_path));
		if (!object) {
			return Event::pre_process_done(Status::NOT_FOUND, _path);
		}

		if (object->parent_graph() != _parent
		    && object->parent()->parent_graph() != _parent) {
			return Event::pre_process_done(Status::INVALID_PARENT, _parent_path);
		}

		// Only one of these will succeed
		_block = dynamic_cast<BlockImpl*>(object);
		_port  = dynamic_cast<PortImpl*>(object);

		if (!_block && !_port) {
			return Event::pre_process_done(Status::INTERNAL_ERROR, _path);
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
			                 dynamic_cast<OutputPort*>(a->tail()),
			                 dynamic_cast<InputPort*>(a->head())));
	}

	if (!_deleting && _parent->enabled())
		_compiled_graph = _parent->compile();

	return Event::pre_process_done(Status::SUCCESS);
}

void
DisconnectAll::execute(ProcessContext& context)
{
	if (_status == Status::SUCCESS) {
		for (auto& i : _impls) {
			i->execute(context,
			           !_deleting || (i->head()->parent_block() != _block));
		}
	}

	_parent->set_compiled_graph(_compiled_graph);
}

void
DisconnectAll::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS) {
		_engine.broadcaster()->disconnect_all(_parent_path, _path);
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
