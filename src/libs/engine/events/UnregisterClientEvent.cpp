/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include "UnregisterClientEvent.h"
#include "Responder.h"
#include "Ingen.h"
#include "ClientBroadcaster.h"
#include "interface/ClientInterface.h"

namespace Ingen {


UnregisterClientEvent::UnregisterClientEvent(CountedPtr<Responder> responder, SampleCount timestamp, ClientKey key)
: QueuedEvent(responder, timestamp)
, _key(key)
{
}


void
UnregisterClientEvent::post_process()
{
	if (Ingen::instance().client_broadcaster()->unregister_client(_key))
		_responder->respond_ok();
	else
		_responder->respond_error("Unable to unregister client");
}


} // namespace Ingen

