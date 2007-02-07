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

#ifndef REGISTERCLIENTEVENT_H
#define REGISTERCLIENTEVENT_H

#include "QueuedEvent.h"
#include "interface/ClientKey.h"
#include "interface/ClientInterface.h"
#include <string>
using std::string;
using Ingen::Shared::ClientInterface;
using Ingen::Shared::ClientKey;

namespace Ingen {


/** Registers a new client with the OSC system, so it can receive updates.
 *
 * \ingroup engine
 */
class RegisterClientEvent : public QueuedEvent
{
public:
	RegisterClientEvent(Engine& engine, SharedPtr<Responder>       responder,
	                    SampleCount                 timestamp,
	                    ClientKey                   key,
	                    SharedPtr<ClientInterface> client);

	void pre_process();
	void post_process();

private:
	ClientKey                   _key;
	SharedPtr<ClientInterface> _client;
};


} // namespace Ingen

#endif // REGISTERCLIENTEVENT_H
