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

#include "Request.hpp"
#include "events/RegisterClient.hpp"
#include "Engine.hpp"
#include "ClientBroadcaster.hpp"

using namespace Raul;

namespace Ingen {
namespace Events {


RegisterClient::RegisterClient(Engine&                  engine,
                               SharedPtr<Request>       request,
                               SampleCount              timestamp,
                               const URI&               uri,
                               Shared::ClientInterface* client)
	: QueuedEvent(engine, request, timestamp)
	, _uri(uri)
	, _client(client)
{
}


void
RegisterClient::pre_process()
{
	_engine.broadcaster()->register_client(_uri, _client);

	QueuedEvent::pre_process();
}


void
RegisterClient::post_process()
{
	_request->respond_ok();
}


} // namespace Ingen
} // namespace Events

