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

#ifndef RESPONDER_H
#define RESPONDER_H

#include <inttypes.h>
#include <string>
#include "util/CountedPtr.h"
#include "interface/ClientKey.h"
using std::string;

namespace Ingen {

using Shared::ClientKey;


/** Class to handle responding to clients.
 *
 * This is an abstract base class to fully abstract the details of client
 * communication from the internals of the engine.
 *
 * Note that this class only handles sending responses to commands from
 * clients, (ie OK or an error), <b>not</b> notifications (ie new node,
 * disconnection) - that's what ClientInterface is for.  If a command is
 * a request, the ClientKey of the Responder can be used to find the
 * ClientInterface which should receive the response.
 *
 * ClientInterface and Responder are seperate because responding might not
 * actually get exposed to the client interface (eg in simulated blocking
 * interfaces that wait for responses before returning).
 */
class Responder
{
public:
	Responder() {}
	virtual ~Responder() {}

	virtual ClientKey client_key() { return ClientKey(); }

	virtual void respond_ok() {}
	virtual void respond_error(const string& msg) {}
};


} // namespace Ingen

#endif // RESPONDER_H

