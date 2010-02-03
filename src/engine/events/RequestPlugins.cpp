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

#include "RequestPlugins.hpp"
#include "Request.hpp"
#include "Engine.hpp"
#include "ClientBroadcaster.hpp"
#include "NodeFactory.hpp"

namespace Ingen {
namespace Events {


RequestPlugins::RequestPlugins(Engine& engine, SharedPtr<Request> request, SampleCount timestamp)
    : QueuedEvent(engine, request, timestamp)
{
}


void
RequestPlugins::pre_process()
{
	// Take a copy to send in the post processing thread (to avoid problems
	// because std::map isn't thread safe)
	_plugins = _engine.node_factory()->plugins();

	QueuedEvent::pre_process();
}


void
RequestPlugins::post_process()
{
	if (_request->client()) {
		_engine.broadcaster()->send_plugins_to(_request->client(), _plugins);
		_request->respond_ok();
	} else {
		_request->respond_error("Unable to find client to send plugins");
	}
}


} // namespace Ingen
} // namespace Events

