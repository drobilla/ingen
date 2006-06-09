/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef REGISTERCLIENTEVENT_H
#define REGISTERCLIENTEVENT_H

#include "QueuedEvent.h"
#include "interface/ClientKey.h"
#include "interface/ClientInterface.h"
#include <string>
using std::string;
using Om::Shared::ClientInterface;
using Om::Shared::ClientKey;

namespace Om {


/** Registers a new client with the OSC system, so it can receive updates.
 *
 * \ingroup engine
 */
class RegisterClientEvent : public QueuedEvent
{
public:
	RegisterClientEvent(CountedPtr<Responder>       responder,
	                    ClientKey                   key,
	                    CountedPtr<ClientInterface> client);

	void pre_process();
	void post_process();

private:
	ClientKey                   _key;
	CountedPtr<ClientInterface> _client;
};


} // namespace Om

#endif // REGISTERCLIENTEVENT_H
