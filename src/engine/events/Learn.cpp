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

#include "events/Learn.hpp"
#include "ClientBroadcaster.hpp"
#include "ControlBindings.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "NodeImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"
#include "Request.hpp"
#include "internals/Controller.hpp"

using namespace std;

namespace Ingen {
namespace Events {


Learn::Learn(Engine& engine, SharedPtr<Request> request, SampleCount timestamp, const Raul::Path& path)
	: QueuedEvent(engine, request, timestamp)
	, _error(NO_ERROR)
	, _path(path)
	, _object(NULL)
	, _done(false)
{
}


void
Learn::pre_process()
{
	_object = _engine.engine_store()->find_object(_path);

	PortImpl* port = dynamic_cast<PortImpl*>(_object);
	if (port) {
		_done = true;
		if (port->type() == Shared::PortType::CONTROL)
			_engine.control_bindings()->learn(port);
	}

	QueuedEvent::pre_process();
}


void
Learn::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_done || !_object)
		return;

	NodeImpl* node = dynamic_cast<NodeImpl*>(_object);
	if (node) {
		if (node->plugin_impl()->type() == Shared::Plugin::Internal) {
			((NodeBase*)_object)->learn();
		} else {
			_error = INVALID_NODE_TYPE;
		}
	}
}


void
Learn::post_process()
{
	if (_error == NO_ERROR) {
		_request->respond_ok();
	} else if (_object == NULL) {
		string msg = "Did not find node '";
		msg.append(_path.str()).append("' for learn.");
		_request->respond_error(msg);
	} else {
		const string msg = string("Object '") + _path.str() + "' is not capable of learning.";
		_request->respond_error(msg);
	}
}


} // namespace Ingen
} // namespace Events


