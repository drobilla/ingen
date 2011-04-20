/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include <boost/format.hpp>

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
#include "Request.hpp"
#include "events/Disconnect.hpp"
#include "events/DisconnectAll.hpp"
#include "util.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Engine {
namespace Events {

DisconnectAll::DisconnectAll(Engine& engine, SharedPtr<Request> request, SampleCount timestamp, const Path& parent_path, const Path& node_path)
	: QueuedEvent(engine, request, timestamp)
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
DisconnectAll::DisconnectAll(Engine& engine, PatchImpl* parent, GraphObjectImpl* object)
	: QueuedEvent(engine)
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
DisconnectAll::remove_connection(ConnectionImpl* c)
{
	_impls.push_back(
		new Disconnect::Impl(_engine,
		                     _parent,
		                     dynamic_cast<OutputPort*>(c->src_port()),
		                     dynamic_cast<InputPort*>(c->dst_port())));
}

void
DisconnectAll::pre_process()
{
	if (!_deleting) {
		_parent = _engine.engine_store()->find_patch(_parent_path);

		if (_parent == NULL) {
			_error = PARENT_NOT_FOUND;
			QueuedEvent::pre_process();
			return;
		}

		GraphObjectImpl* object = _engine.engine_store()->find_object(_path);

		if (object == NULL) {
			_error = OBJECT_NOT_FOUND;
			QueuedEvent::pre_process();
			return;
		}

		if (object->parent_patch() != _parent && object->parent()->parent_patch() != _parent) {
			_error = INVALID_PARENT_PATH;
			QueuedEvent::pre_process();
			return;
		}

		// Only one of these will succeed
		_node = dynamic_cast<NodeImpl*>(object);
		_port = dynamic_cast<PortImpl*>(object);

		assert((_node || _port) && !(_node && _port));
	}

	if (_node) {
		for (PatchImpl::Connections::const_iterator i = _parent->connections().begin();
				i != _parent->connections().end(); ++i) {
			ConnectionImpl* c = (ConnectionImpl*)i->second.get();
			if (!c->pending_disconnection()
			    && (c->src_port()->parent_node() == _node
			        || c->dst_port()->parent_node() == _node)) {
				remove_connection(c);
			}
		}
	} else { // _port
		for (PatchImpl::Connections::const_iterator i = _parent->connections().begin();
				i != _parent->connections().end(); ++i) {
			ConnectionImpl* c = (ConnectionImpl*)i->second.get();
			if (!c->pending_disconnection()
			    && (c->src_port() == _port
			        || c->dst_port() == _port)) {
				remove_connection(c);
			}
		}
	}

	if (!_deleting && _parent->enabled())
		_compiled_patch = _parent->compile();

	QueuedEvent::pre_process();
}

void
DisconnectAll::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_error == NO_ERROR) {
		for (Impls::iterator i = _impls.begin(); i != _impls.end(); ++i) {
			(*i)->execute(context,
			              !_deleting || ((*i)->dst_port()->parent_node() != _node));
		}
	}

	_engine.maid()->push(_parent->compiled_patch());
	_parent->compiled_patch(_compiled_patch);
}

void
DisconnectAll::post_process()
{
	if (_error == NO_ERROR) {
		if (_request)
			_request->respond_ok();
		_engine.broadcaster()->disconnect_all(_parent_path, _path);
	} else {
		if (_request) {
			boost::format fmt("Unable to disconnect %1% (%2%)");
			fmt % _path;
			switch (_error) {
				case INVALID_PARENT_PATH:
					fmt % string("Invalid parent path: ").append(_parent_path.str());
					break;
				case PARENT_NOT_FOUND:
					fmt % string("Unable to find parent: ").append(_parent_path.str());
					break;
				case OBJECT_NOT_FOUND:
					fmt % string("Unable to find object");
				default:
					break;
			}
			_request->respond_error(fmt.str());
		}
	}
}

} // namespace Engine
} // namespace Ingen
} // namespace Events

