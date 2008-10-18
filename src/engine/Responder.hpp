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

#ifndef RESPONDER_H
#define RESPONDER_H

#include <inttypes.h>
#include <string>
#include "raul/SharedPtr.hpp"
#include "interface/ClientInterface.hpp"

namespace Ingen {


/** Class to handle responding to clients.
 *
 * This is a glorified std::pair<ClientInterface*, int32_t> for replying
 * to numbered messages from a client.
 *
 * For responses that involve more messages, the response will come first
 * followed by the messages (eg object notifications, values, errors, etc.)
 * in a bundle (or "transfer" if too large).
 */
class Responder
{
public:
	Responder(Shared::ClientInterface* client=0, int32_t id=1)
		: _client(client)
		, _id(id)
	{}

	int32_t id() const { return _id; }
	void    set_id(int32_t id) { _id = id; }

	Shared::ClientInterface* client() const { return _client; }
	void set_client(Shared::ClientInterface* client) { _client = client; }

	void respond_ok() {
		if (_client)
			_client->response_ok(_id);
	}

	void respond_error(const std::string& msg) {
		if (_client)
			_client->response_error(_id, msg);
	}

private:
	Shared::ClientInterface* _client;
	int32_t                  _id;
};


} // namespace Ingen

#endif // RESPONDER_H

