/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_OBJECTSENDER_HPP
#define INGEN_ENGINE_OBJECTSENDER_HPP

namespace Ingen {

class Interface;

namespace Server {

class GraphObjectImpl;
class PatchImpl;
class NodeImpl;
class PortImpl;
class PluginImpl;

/** Utility class for sending GraphObjects to clients via Interface.
 *
 * While Interface is the direct low level message-based interface
 * (protocol), this is used from the engine to easily send proper Objects
 * with these messages (which is done in a few different parts of the code).
 *
 * Basically a serialiser, except to calls on Interface rather than
 * eg a byte stream.
 */
class ObjectSender {
public:
	static void send_object(Interface*             client,
	                        const GraphObjectImpl* object,
	                        bool                   recursive);

private:
	static void send_patch(Interface*       client,
	                       const PatchImpl* patch,
	                       bool             recursive,
	                       bool             bundle = true);
	static void send_node(Interface*        client,
	                      const NodeImpl*   node,
	                      bool              recursive,
	                      bool              bundle = true);
	static void send_port(Interface*        client,
	                      const PortImpl*   port,
	                      bool              bundle = true);
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_OBJECTSENDER_HPP

