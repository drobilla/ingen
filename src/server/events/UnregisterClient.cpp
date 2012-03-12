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

#include "ingen/ClientInterface.hpp"
#include "UnregisterClient.hpp"
#include "Engine.hpp"
#include "ClientBroadcaster.hpp"

using namespace Raul;

namespace Ingen {
namespace Server {
namespace Events {

UnregisterClient::UnregisterClient(Engine&          engine,
                                   ClientInterface* client,
                                   int32_t          id,
                                   SampleCount      timestamp,
                                   const URI&       uri)
	: Event(engine, client, id, timestamp)
	, _uri(uri)
{
}

void
UnregisterClient::post_process()
{
	if (_engine.broadcaster()->unregister_client(_uri)) {
		respond(SUCCESS);
	} else {
		respond(FAILURE);
	}
}

} // namespace Server
} // namespace Ingen
} // namespace Events

