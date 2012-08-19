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

#include <set>

#include <boost/format.hpp>
#include <glibmm/thread.h>

#include "ingen/Store.hpp"
#include "raul/Array.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"

#include "BlockImpl.hpp"
#include "Broadcaster.hpp"
#include "EdgeImpl.hpp"
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

DisconnectAll::DisconnectAll(Engine&              engine,
                             SharedPtr<Interface> client,
                             int32_t              id,
                             SampleCount          timestamp,
                             const Raul::Path&    parent_path,
                             const Raul::Path&    path)
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
	for (Impls::iterator i = _impls.begin(); i != _impls.end(); ++i)
		delete (*i);
}

bool
DisconnectAll::pre_process()
{
	Glib::RWLock::WriterLock lock(_engine.store()->lock(), Glib::NOT_LOCK);

	if (!_deleting) {
		lock.acquire();

		_parent = dynamic_cast<GraphImpl*>(_engine.store()->get(_parent_path));
		if (!_parent) {
			return Event::pre_process_done(PARENT_NOT_FOUND, _parent_path);
		}

		NodeImpl* const object = dynamic_cast<NodeImpl*>(
			_engine.store()->get(_path));
		if (!object) {
			return Event::pre_process_done(NOT_FOUND, _path);
		}

		if (object->parent_graph() != _parent
		    && object->parent()->parent_graph() != _parent) {
			return Event::pre_process_done(INVALID_PARENT_PATH, _parent_path);
		}

		// Only one of these will succeed
		_block = dynamic_cast<BlockImpl*>(object);
		_port  = dynamic_cast<PortImpl*>(object);

		assert((_block || _port) && !(_block && _port));
	}

	// Find set of edges to remove
	std::set<EdgeImpl*> to_remove;
	for (Node::Edges::const_iterator i = _parent->edges().begin();
	     i != _parent->edges().end(); ++i) {
		EdgeImpl* const c = (EdgeImpl*)i->second.get();
		if (_block) {
			if (c->tail()->parent_block() == _block
			    || c->head()->parent_block() == _block) {
				to_remove.insert(c);
			}
		} else {
			assert(_port);
			if (c->tail() == _port || c->head() == _port) {
				to_remove.insert(c);
			}
		}
	}

	// Create disconnect events (which erases from _parent->edges())
	for (std::set<EdgeImpl*>::const_iterator i = to_remove.begin();
	     i != to_remove.end(); ++i) {
		_impls.push_back(new Disconnect::Impl(
			                 _engine, _parent,
			                 dynamic_cast<OutputPort*>((*i)->tail()),
			                 dynamic_cast<InputPort*>((*i)->head())));
	}

	if (!_deleting && _parent->enabled())
		_compiled_graph = _parent->compile();

	return Event::pre_process_done(SUCCESS);
}

void
DisconnectAll::execute(ProcessContext& context)
{
	if (_status == SUCCESS) {
		for (Impls::iterator i = _impls.begin(); i != _impls.end(); ++i) {
			(*i)->execute(context,
			              !_deleting || ((*i)->head()->parent_block() != _block));
		}
	}

	_engine.maid()->dispose(_parent->compiled_graph());
	_parent->compiled_graph(_compiled_graph);
}

void
DisconnectAll::post_process()
{
	if (!respond()) {
		_engine.broadcaster()->disconnect_all(_parent_path, _path);
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen

