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

#ifndef CLIENTKEY_H
#define CLIENTKEY_H

#include <string>

namespace Ingen {
namespace Shared {


/** Key for looking up clients.
 *
 * This might actually be a lookup key (as in OSC clients) or a direct handle
 * of some kind (eg pointer) (for in-process or shared memory clients).
 *
 * This is shared code, nothing client or server code specific should be here.
 */
class ClientKey
{
public:
	/** A key to identify a particular ClientInterface.
	 *
	 * There are two different OSC key types because one is for server side,
	 * and one is for client side.  A client can not identify it's own URL to
	 * the host (for NAT traversal, etc) so it can only know the outgoing UDP
	 * port it's sending on.  The server however identifies that client by the
	 * full incoming URL.
	 */
	enum Type {
		NIL,     ///< A NULL key (represents no client)
		OSC_URL, ///< An OSC URL (remote host/client)
		OSC_PORT ///< A local OSC port
	};
	
	ClientKey()
	: _type(NIL),
	  _uri("")
	{}

	ClientKey(Type type, const std::string& uri)
	: _type(OSC_URL)
	, _uri(uri)
	{}
	
	/*ClientKey(Type type, int port)
	: _type(OSC_PORT)
	, _port(port)
	{}*/

	inline bool
	operator==(const ClientKey& key) const
	{
		return (_type == key._type && _uri == key._uri);
	}
	
	inline Type               type() const { return _type; }
	inline const std::string& uri()  const { return _uri; }

protected:
	Type              _type;
	const std::string _uri; ///< URL for OSC_URL, (string) port number for OSC_PORT
};


} // namespace Shared
} // namespace Ingen

#endif // CLIENTKEY_H

