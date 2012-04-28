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

#include "raul/Array.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"

#include "ClientBroadcaster.hpp"
#include "ConnectionImpl.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "InputPort.hpp"
#include "NodeImpl.hpp"
#include "OutputPort.hpp"
#include "PatchImpl.hpp"
#include "PortImpl.hpp"
#include "events/Disconnect.hpp"
#include "events/DisconnectAll.hpp"
#include "util.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {
namespace Events {

DisconnectAll::DisconnectAll(Engine&     engine,
                             Interface*  client,
                             int32_t     id,
                             SampleCount timestamp,
                             const Path& parent_path,
                             const Path& node_path)
	: Event(engine, client, id, timestamp)
	, _parent_path(parent_path)
	, _path(node_path)
	, _parent(NULL)
	, _node(NULL)
	, _port(NULL)
	, _compiled_patch(NULL)
	, _deleting(false)
{
}

/** Internal version for use by other events.
 */
DisconnectAll::DisconnectAll(Engine&          engine,
                             PatchImpl*       parent,
                             GraphObjectImpl* object)
	: Event(engine)
	, _parent_path(parent->path())
	, _path(object->path())
	, _parent(parent)
	, _node(dynamic_cast<NodeImpl*>(object))
	, _port(dynamic_cast<PortImpl*>(object))
	, _compiled_patch(NULL)
	, _deleting(true)
{
}

DisconnectAll::~DisconnectAll()
{
	for (Impls::iterator i = _impls.begin(); i != _impls.end(); ++i)
		delete (*i);
}

void
DisconnectAll::pre_process()
{
	Glib::RWLock::WriterLock lock(_engine.engine_store()->lock(), Glib::NOT_LOCK);

	if (!_deleting) {
		lock.acquire();

		_parent = _engine.engine_store()->find_patch(_parent_path);

		if (_parent == NULL) {
			_status = PARENT_NOT_FOUND;
			Event::pre_process();
			return;
		}

		GraphObjectImpl* object = _engine.engine_store()->find_object(_path);
		if (!object) {
			_status = NOT_FOUND;
			Event::pre_process();
			return;
		}

		if (object->parent_patch() != _parent
		    && object->parent()->parent_patch() != _parent) {
			_status = INVALID_PARENT_PATH;
			Event::pre_process();
			return;
		}

		// Only one of these will succeed
		_node = dynamic_cast<NodeImpl*>(object);
		_port = dynamic_cast<PortImpl*>(object);

		assert((_node || _port) && !(_node && _port));
	}

	// Find set of connections to remove
	std::set<ConnectionImpl*> to_remove;
	for (Patch::Connections::const_iterator i = _parent->connections().begin();
	     i != _parent->connections().end(); ++i) {
		ConnectionImpl* const c = (ConnectionImpl*)i->second.get();
		if (_node) {
			if (c->tail()->parent_node() == _node
			    || c->head()->parent_node() == _node) {
				to_remove.insert(c);
			}
		} else {
			assert(_port);
			if (c->tail() == _port || c->head() == _port) {
				to_remove.insert(c);
			}
		}
	}

	// Create disconnect events (which erases from _parent->connections())
	for (std::set<ConnectionImpl*>::const_iterator i = to_remove.begin();
	     i != to_remove.end(); ++i) {
		_impls.push_back(new Disconnect::Impl(
			                 _engine, _parent,
			                 dynamic_cast<OutputPort*>((*i)->tail()),
			                 dynamic_cast<InputPort*>((*i)->head())));
	}

	if (!_deleting && _parent->enabled())
		_compiled_patch = _parent->compile();

	Event::pre_process();
}

void
DisconnectAll::execute(ProcessContext& context)
{
	Event::execute(context);

	if (_status == SUCCESS) {
		for (Impls::iterator i = _impls.begin(); i != _impls.end(); ++i) {
			(*i)->execute(context,
			              !_deleting || ((*i)->head()->parent_node() != _node));
		}
	}

	_engine.maid()->push(_parent->compiled_patch());
	_parent->compiled_patch(_compiled_patch);
}

void
DisconnectAll::post_process()
{
	respond(_status);
	if (!_status) {
		_engine.broadcaster()->disconnect_all(_parent_path, _path);
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen

