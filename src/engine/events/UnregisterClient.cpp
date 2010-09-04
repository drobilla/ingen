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

#include "interface/ClientInterface.hpp"
#include "Request.hpp"
#include "UnregisterClient.hpp"
#include "Engine.hpp"
#include "ClientBroadcaster.hpp"

using namespace Raul;

namespace Ingen {
namespace Events {


UnregisterClient::UnregisterClient(Engine& engine, SharedPtr<Request> request, SampleCount timestamp, const URI& uri)
	: QueuedEvent(engine, request, timestamp)
	, _uri(uri)
{
}


void
UnregisterClient::post_process()
{
	if (_engine.broadcaster()->unregister_client(_uri))
		_request->respond_ok();
	else
		_request->respond_error("Unable to unregister client");
}


} // namespace Ingen
} // namespace Events

