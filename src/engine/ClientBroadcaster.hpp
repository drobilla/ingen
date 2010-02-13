/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#include <string>
#include <list>
#include <map>
#include <pthread.h>
#include "raul/SharedPtr.hpp"
#include "interface/ClientInterface.hpp"
#include "NodeFactory.hpp"

#include <iostream>
using namespace std;

namespace Ingen {

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
class ClientBroadcaster : public Shared::ClientInterface
{
public:
	void register_client(const Raul::URI& uri, Shared::ClientInterface* client);
	bool unregister_client(const Raul::URI& uri);

	Shared::ClientInterface* client(const Raul::URI& uri);

	void send_plugins(const NodeFactory::Plugins& plugin_list);
	void send_plugins_to(Shared::ClientInterface*, const NodeFactory::Plugins& plugin_list);

	void send_object(const GraphObjectImpl* p, bool recursive);

#define BROADCAST(msg, ...) \
	for (Clients::const_iterator i = _clients.begin(); i != _clients.end(); ++i) \
		(*i).second->msg(__VA_ARGS__)


	// CommonInterface

	void bundle_begin() { BROADCAST(bundle_begin); }
	void bundle_end()   { BROADCAST(bundle_end); }

	void put(const Raul::URI&                    uri,
	         const Shared::Resource::Properties& properties) {
		BROADCAST(put, uri, properties);
	}

	void delta(const Raul::URI&                    uri,
	           const Shared::Resource::Properties& remove,
	           const Shared::Resource::Properties& add) {
		BROADCAST(delta, uri, remove, add);
	}

	void move(const Raul::Path& old_path,
	          const Raul::Path& new_path) {
		BROADCAST(move, old_path, new_path);
	}

	void del(const Raul::Path& path) {
		BROADCAST(del, path);
	}

	void connect(const Raul::Path& src_port_path,
	             const Raul::Path& dst_port_path) {
		BROADCAST(connect, src_port_path, dst_port_path);
	}

	void disconnect(const Raul::Path& src_port_path,
	                const Raul::Path& dst_port_path) {
		BROADCAST(disconnect, src_port_path, dst_port_path);
	}

	void set_property(const Raul::URI&  subject,
	                  const Raul::URI&  predicate,
	                  const Raul::Atom& value) {
		BROADCAST(set_property, subject, predicate, value);
	}

	void set_voice_value(const Raul::Path& port_path,
	                     uint32_t          voice,
	                     const Raul::Atom& value) {
		BROADCAST(set_voice_value, port_path, voice, value);
	}


	// ClientInterface

	Raul::URI uri() const { return "http://drobilla.net/ns/ingen#broadcaster"; } ///< N/A

	void response_ok(int32_t id) {} ///< N/A
	void response_error(int32_t id, const std::string& msg) {} ///< N/A

	void transfer_begin()                 { BROADCAST(transfer_begin); }
	void transfer_end()                   { BROADCAST(transfer_end); }
	void error(const std::string& msg)    { BROADCAST(error, msg); }
	void activity(const Raul::Path& path) { BROADCAST(activity, path); }

private:
	typedef std::map<Raul::URI, Shared::ClientInterface*> Clients;
	Clients _clients;
};


} // namespace Ingen

#endif // INGEN_ENGINE_CLIENTBROADCASTER_HPP

