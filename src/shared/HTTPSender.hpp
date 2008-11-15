/* This file is part of Ingen.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
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

#ifndef HTTPSENDER_H
#define HTTPSENDER_H

#include <stdint.h>
#include <string>
#include <libsoup/soup.h>

namespace Ingen {
namespace Shared {

class HTTPSender {
public:
	HTTPSender(SoupServer* server, SoupMessage* msg);
	virtual ~HTTPSender();

	// Message bundling
	void bundle_begin();
	void bundle_end();
	
	// Transfers (loose bundling)
	void transfer_begin() { bundle_begin(); }
	void transfer_end()   { bundle_end(); }

protected:
	void send_chunk(const std::string& buf);

	enum SendState { Immediate, SendingBundle };

	SoupServer*  _server;
	SoupMessage* _msg;
	SendState    _send_state;
	std::string  _transfer;
};


} // namespace Shared
} // namespace Ingen

#endif // HTTPSENDER_H

