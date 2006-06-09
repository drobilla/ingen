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

#ifndef OSCRESPONDER_H
#define OSCRESPONDER_H

#include <inttypes.h>
#include <memory>
#include <lo/lo.h>
#include "Responder.h"

namespace Om {


/** Responder for (liblo) OSC clients.
 *
 * OSC Clients use request IDs to be able to associate replies with sent
 * events.  If the ID is -1, a response will not be sent (and the overhead
 * of searching for the client's record will be skipped).  Any other integer
 * is a valid response ID and will be responded to.
 *
 * Creation of the lo_address is deferred until needed to avoid bogging down
 * the receiving thread as much as possible.
 */
class OSCResponder : public Responder
{
public:
	OSCResponder(int32_t id, char* url);
	~OSCResponder();
	
	CountedPtr<Shared::ClientInterface> find_client();
	
	void respond_ok();
	void respond_error(const string& msg);

	const char* url() const { return _url; }

private:
	int32_t     _id;
	char* const _url;
	lo_address  _addr;
};


} // namespace Om

#endif // OSCRESPONDER_H

