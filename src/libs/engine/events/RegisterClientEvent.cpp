/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include "interface/Responder.h"
#include "RegisterClientEvent.h"
#include "Engine.h"
#include "ClientBroadcaster.h"

namespace Ingen {


RegisterClientEvent::RegisterClientEvent(Engine& engine, SharedPtr<Shared::Responder>       responder,
                                         SampleCount                 timestamp,
                                         ClientKey                   key,
                                         SharedPtr<ClientInterface> client)
: QueuedEvent(engine, responder, timestamp)
, _key(key)
, _client(client)
{
}


void
RegisterClientEvent::pre_process()
{  
	_engine.broadcaster()->register_client(_key, _client);

	QueuedEvent::pre_process();
}


void
RegisterClientEvent::post_process()
{
	_responder->respond_ok();
}


} // namespace Ingen

