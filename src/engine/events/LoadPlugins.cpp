/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#include "LoadPlugins.hpp"
#include "Request.hpp"
#include "Engine.hpp"
#include "NodeFactory.hpp"
#include "ClientBroadcaster.hpp"

namespace Ingen {
namespace Events {


LoadPlugins::LoadPlugins(Engine& engine, SharedPtr<Request> request, SampleCount timestamp)
	: QueuedEvent(engine, request, timestamp, bool(request))
{
}

void
LoadPlugins::pre_process()
{
	_engine.node_factory()->load_plugins();

	QueuedEvent::pre_process();
}

void
LoadPlugins::post_process()
{
	if (_request)
		_request->unblock();

	_request->respond_ok();
}


} // namespace Ingen
} // namespace Events

