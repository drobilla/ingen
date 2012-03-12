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

#ifndef INGEN_ENGINE_CLIENTBROADCASTER_HPP
#define INGEN_ENGINE_CLIENTBROADCASTER_HPP

#include <list>
#include <map>
#include <string>

#include "raul/SharedPtr.hpp"

#include "ingen/ClientInterface.hpp"

#include "NodeFactory.hpp"

using namespace std;

namespace Ingen {
namespace Server {

class GraphObjectImpl;
class NodeImpl;
class PortImpl;
class PluginImpl;
class PatchImpl;
class ConnectionImpl;

/** Broadcaster for all clients.
 *
 * This is a ClientInterface that forwards all messages to all registered
 * clients (for updating all clients on state changes in the engine).
 *
 * \ingroup engine
 */
class ClientBroadcaster : public ClientInterface
{
public:
	void register_client(const Raul::URI& uri, ClientInterface* client);
	bool unregister_client(const Raul::URI& uri);

	ClientInterface* client(const Raul::URI& uri);

	void send_plugins(const NodeFactory::Plugins& plugin_list);
	void send_plugins_to(ClientInterface*, const NodeFactory::Plugins& plugin_list);

	void send_object(const GraphObjectImpl* p, bool recursive);

#define BROADCAST(msg, ...) \
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i) \
		(*i).second->msg(__VA_ARGS__)

	// CommonInterface

	void bundle_begin() { BROADCAST(bundle_begin); }
	void bundle_end()   { BROADCAST(bundle_end); }

	void put(const Raul::URI&            uri,
	         const Resource::Properties& properties,
	         Resource::Graph             ctx=Resource::DEFAULT) {
		BROADCAST(put, uri, properties);
	}

	void delta(const Raul::URI&            uri,
	           const Resource::Properties& remove,
	           const Resource::Properties& add) {
		BROADCAST(delta, uri, remove, add);
	}

	void move(const Raul::Path& old_path,
	          const Raul::Path& new_path) {
		BROADCAST(move, old_path, new_path);
	}

	void del(const Raul::URI& uri) {
		BROADCAST(del, uri);
	}

	void connect(const Raul::Path& src_port_path,
	             const Raul::Path& dst_port_path) {
		BROADCAST(connect, src_port_path, dst_port_path);
	}

	void disconnect(const Raul::URI& src,
	                const Raul::URI& dst) {
		BROADCAST(disconnect, src, dst);
	}

	void disconnect_all(const Raul::Path& parent_patch_path,
	                    const Raul::Path& path) {
		BROADCAST(disconnect_all, parent_patch_path, path);
	}

	void set_property(const Raul::URI&  subject,
	                  const Raul::URI&  predicate,
	                  const Raul::Atom& value) {
		BROADCAST(set_property, subject, predicate, value);
	}

	// ClientInterface

	Raul::URI uri() const { return "http://drobilla.net/ns/ingen#broadcaster"; } ///< N/A

	void response(int32_t id, Status status) {} ///< N/A

	void error(const std::string& msg) { BROADCAST(error, msg); }

	void activity(const Raul::Path& path,
	              const Raul::Atom& value) {
		BROADCAST(activity, path, value);
	}

private:
	typedef std::map<Raul::URI, ClientInterface*> Clients;
	Clients _clients;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_CLIENTBROADCASTER_HPP

