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

#ifndef CLIENTBROADCASTER_H
#define CLIENTBROADCASTER_H

#include <string>
#include <list>
#include <map>
#include <pthread.h>
#include "raul/SharedPtr.hpp"
#include "interface/ClientInterface.hpp"
#include "NodeFactory.hpp"

namespace Ingen {

class GraphObjectImpl;
class NodeImpl;
class PortImpl;
class PluginImpl;
class PatchImpl;
class ConnectionImpl;


/** Broadcaster for all clients.
 *
 * This sends messages to all client simultaneously through the opaque
 * ClientInterface.  The clients may be OSC driver, in process, theoretically
 * anything that implements ClientInterface.
 *
 * This also serves as the database of all registered clients.
 *
 * \ingroup engine
 */
class ClientBroadcaster
{
public:
	void register_client(const Raul::URI& uri, Shared::ClientInterface* client);
	bool unregister_client(const Raul::URI& uri);

	Shared::ClientInterface* client(const Raul::URI& uri);

	//void send_client_registration(const string& url, int client_id);

	void bundle_begin();
	void bundle_end();

	// Error that isn't the direct result of a request
	void send_error(const std::string& msg);

	void send_plugins(const NodeFactory::Plugins& plugin_list);
	void send_object(const GraphObjectImpl* p, bool recursive);
	void send_destroyed(const Raul::Path& path);
	void send_clear_patch(const Raul::Path& patch_path);
	void send_connection(const SharedPtr<const ConnectionImpl> connection);
	void send_disconnection(const Raul::Path& src_port_path, const Raul::Path& dst_port_path);
	void send_rename(const Raul::Path& old_path, const Raul::Path& new_path);
	void send_variable_change(const Raul::URI& node_path, const Raul::URI& key, const Raul::Atom& value);
	void send_property_change(const Raul::URI& node_path, const Raul::URI& key, const Raul::Atom& value);
	void send_port_value(const Raul::Path& port_path, const Raul::Atom& value);
	void send_activity(const Raul::Path& path);
	void send_program_add(const Raul::Path& node_path, int bank, int program, const std::string& name);
	void send_program_remove(const Raul::Path& node_path, int bank, int program);

	void send_plugins_to(Shared::ClientInterface*, const NodeFactory::Plugins& plugin_list);

private:
	typedef std::map<Raul::URI, Shared::ClientInterface*> Clients;
	Clients _clients;
};



} // namespace Ingen

#endif // CLIENTBROADCASTER_H

