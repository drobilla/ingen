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

#ifndef OBJECTSENDER_H
#define OBJECTSENDER_H

#include <list>

namespace Ingen {

namespace Shared {
	class ClientInterface;
} using Shared::ClientInterface;

class Patch;
class Node;
class Port;
class Plugin;


/** Utility class for sending GraphObjects to clients through ClientInterface.
 *
 * While ClientInterface is the direct low level message-based interface
 * (protocol), this is used from the engine to easily send proper Objects
 * with these messages (which is done in a few different parts of the code).
 *
 * Basically a serializer, except to calls on ClientInterface rather than
 * eg a byte stream.
 */
class ObjectSender {
public:
	
	// FIXME: Make all object parameters const
	
	static void send_patch(ClientInterface* client, const Patch* patch);
	static void send_node(ClientInterface* client, const Node* node);
	static void send_port(ClientInterface* client, const Port* port);
	static void send_plugins(ClientInterface* client, const std::list<Plugin*>& plugs);
};

} // namespace Ingen

#endif // OBJECTSENDER_H

