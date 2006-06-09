/* This file is part of Om.  Copyright (C) 2005 Dave Robillard.
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

#ifndef OSCSENDER_H
#define OSCSENDER_H

#include <string>
#include <iostream>
#include <list>
#include <lo/lo.h>
#include <pthread.h>
#include "util/types.h"

using std::list; using std::string;

namespace Om {

class Node;
class Port;
class PortInfo;
class Plugin;
class Patch;
class Connection;
class ClientRecord;
class Responder;


/** Sends OSC messages (for two-way communication with client)
 *
 * \ingroup engine
 */
class OSCSender
{
public:
	void          register_client(const string& url);
	ClientRecord* unregister_client(const Responder* responder);
	ClientRecord* client(const Responder* responder);
	
	// Notification band:
	
	//void send_client_registration(const string& url, int client_id);
	
	// Error that isn't the direct result of a request
	void send_error(const string& msg);

	void send_plugins_to(ClientRecord* client);
	
	void send_node_creation_messages(const Node* const node);
	
	// These versions send to all clients, using single client versions below
	void send_patch(const Patch* const p);
	void send_new_port(const Port* port);
	void send_node(const Node* const node);
	void send_destroyed(const string& path);
	void send_patch_cleared(const string& patch_path);
	void send_connection(const Connection* const connection);
	void send_disconnection(const string& src_port_path, const string& dst_port_path);
	void send_rename(const string& old_path, const string& new_path);
	void send_all_objects();
	void send_patch_enable(const string& patch_path);
	void send_patch_disable(const string& patch_path);
	void send_metadata_update(const string& node_path, const string& key, const string& value);
	void send_control_change(const string& port_path, float value);
	void send_program_add(const string& node_path, int bank, int program, const string& name);
	void send_program_remove(const string& node_path, int bank, int program);

	
	// These versions send to passed address
	void send_patch_to(ClientRecord* client, const Patch* const p);
	void send_new_port_to(ClientRecord* client, const Port* port);
	void send_node_to(ClientRecord* client, const Node* const node);
	void send_destroyed_to(ClientRecord* client, const string& path);
	void send_patch_cleared_to(ClientRecord* client, const string& patch_path);
	void send_connection_to(ClientRecord* client, const Connection* const connection);
	void send_disconnection_to(ClientRecord* client, const string& src_port_path, const string& dst_port_path);
	void send_rename_to(ClientRecord* client, const string& old_path, const string& new_path);
	void send_all_objects_to(ClientRecord* client);
	void send_patch_enable_to(ClientRecord* client, const string& patch_path);
	void send_patch_disable_to(ClientRecord* client, const string& patch_path);
	void send_metadata_update_to(ClientRecord* client, const string& node_path, const string& key, const string& value);
	void send_control_change_to(ClientRecord* client, const string& port_path, float value);
	void send_program_add_to(ClientRecord* client, const string& node_path, int bank, int program, const string& name);
	void send_program_remove_to(ClientRecord* client, const string& node_path, int bank, int program);

private:
	list<ClientRecord*> m_clients;
};


} // namespace Om

#endif // OSCSENDER_H
