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

#include "OSCResponder.hpp"
#include "ClientBroadcaster.hpp"
#include <cstdlib>
#include <iostream>
#include <inttypes.h>
#include <lo/lo.h>

using std::cout; using std::cerr; using std::endl;

namespace Ingen {


/** Construct an OSCResponder from \a addr.
 * Takes ownership of @a url.
 */
OSCResponder::OSCResponder(ClientBroadcaster* broadcaster, int32_t id, char* url)
: Responder()
, _broadcaster(broadcaster)
, _id(id)
, _url(url)
, _addr(NULL)
{
	// If ID is -1 this shouldn't have even been created
	assert(id != -1);
}


OSCResponder::~OSCResponder()
{
	//cerr << "DELETING " << _url << " RESPONDER\n";

	if (_addr)
		lo_address_free(_addr);
}


void
OSCResponder::respond_ok()
{
	ClientInterface* client = this->client();
	if (client)
		client->enable();

	 _addr = lo_address_new_from_url(_url);

	//cerr << "OK  " << _id << endl;
	if (lo_send(_addr, "/ingen/response", "iis", _id, 1, "") < 0) {
		cerr << "Unable to send response " << _id << "! ("
			<< lo_address_errstr(_addr) << ")" << endl;
	}
}


void
OSCResponder::respond_error(const string& msg)
{
	ClientInterface* client = this->client();
	if (client)
		client->enable();

	_addr = lo_address_new_from_url(_url);

	//cerr << "ERR " << _id << endl;
	if (lo_send(_addr, "/ingen/response", "iis",_id, 0, msg.c_str()) < 0) {
		cerr << "Unable to send response " << _id << "! ("
			<< lo_address_errstr(_addr) << endl;
	}
}


ClientInterface*
OSCResponder::client()
{
	if (_broadcaster)
		return _broadcaster->client(client_uri());
	else
		return NULL;
}

} // namespace OM

