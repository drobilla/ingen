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

#include "OSCSender.h"
#include <cassert>
#include <iostream>
#include <unistd.h>
#include "Om.h"
#include "OmApp.h"
#include "ObjectStore.h"
#include "NodeFactory.h"
#include "util.h"
#include "Patch.h"
#include "Node.h"
#include "PortInfo.h"
#include "Plugin.h"
#include "PortBase.h"
#include "Connection.h"
#include "AudioDriver.h"
#include "ClientRecord.h"
#include "OSCResponder.h"

using std::cout; using std::cerr; using std::endl;

namespace Om {

	
/*! \page client_osc_namespace Client OSC Namespace Documentation
 *
 * <p>These are all the messages sent from the engine to the client.  Om
 * communication takes place over two distinct bands: control band and
 * notification band.</p>
 * <p>The control band is where clients send commands, and receive a simple
 * response, either OK or an error.</p>
 * <p>All notifications of engine state (ie new nodes) are sent over the
 * notification band <em>which is seperate from the control band</em>.  The
 * reasoning behind this is that many clients may be connected at the same
 * time - a client may receive notifications that are not a direct consequence
 * of some message it sent.</p>
 * <p>The notification band can be thought of as a stream of events representing
 * the changing engine state.  For example, It is possible for a client to send
 * commands and receive aknowledgements, and not listen to the notification band
 * at all; or (in the near future anyway) for a client to use UDP for the control
 * band (for speed), and TCP for the notification band (for reliability and
 * order guarantees).</p>
 * \n\n
 */

	
/* Documentation for namespace portion implemented in OSCResponder.cpp */
	
/** \page client_osc_namespace
 * \n
 * <h3>Notification Band</h3>
 */

/** \page client_osc_namespace
 * <p> \b /om/response/ok - Respond successfully to a user command
 * \arg \b responder-id (int) - OSCResponder ID this is a response to
 * </p> \n \n
 */
	
/** \page client_osc_namespace
 * <p> \b /om/response/error - Respond negatively to a user command
 * \arg \b responder-id (int) - OSCResponder ID this is a response to
 * \arg \b message (string) - Error message (natural language text)
 * </p> \n \n
 */


	
/** Register a client to receive messages over the notification band.
 */
void
OSCSender::register_client(const string& url)
{
	bool found = false;
	for (list<ClientRecord*>::iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		if ((*i)->url() == url)
			found = true;

	if (!found) {
		ClientRecord* cr = new ClientRecord(url);
		m_clients.push_back(cr);
		cout << "[OSC] Registered client " << url << " (" << m_clients.size() << " clients)" << endl;
	} else {
		cout << "[OSC] Client already registered." << endl;
	}
}


/** Remove a client from the list of registered clients.
 *
 * The removed client is returned (not deleted).  It is the caller's
 * responsibility to delete the returned pointer, if it's not NULL.
 *
 * FIXME: Using a "Responder" as the key for client lookups is less than clear.
 */
ClientRecord*
OSCSender::unregister_client(const Responder* responder)
{
	if (responder == NULL)
		return NULL;

	// FIXME: remove filthy cast
	const string url = lo_address_get_url(((OSCResponder*)responder)->source());
	ClientRecord* r = NULL;

	for (list<ClientRecord*>::iterator i = m_clients.begin(); i != m_clients.end(); ++i) {
		if ((*i)->url() == url) {
			r = *i;
			m_clients.erase(i);
			break;
		}
	}
	
	if (r != NULL)
		cout << "[OSC] Unregistered client " << r->url() << " (" << m_clients.size() << " clients)" << endl;
	else
		cerr << "[OSC] ERROR: Unable to find client to unregister!" << endl;
	
	return r;
}


/** Looks up the client with the given @a source address (which is used as the
 * unique identifier for registered clients).
 *
 * (A responder is passed to remove the dependency on liblo addresses in request
 * events, in anticipation of libom and multiple ways of responding to clients).
 *
 * FIXME: Using a "Responder" as the key for client lookups is less than clear.
 */
ClientRecord*
OSCSender::client(const Responder* responder)
{
	// FIXME: eliminate filthy cast
	const string url = lo_address_get_url(((OSCResponder*)responder)->source());
	
	for (list<ClientRecord*>::iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		if ((*i)->url() == url)
			return (*i);

	return NULL;
}

/** \page client_osc_namespace
 * \n
 * <h3>Notification Band</h3>
 */

/** \page client_osc_namespace
 * <p> \b /om/client_registration - Notification that a new client has registered
 * \arg \b url (string) - URL notifications will be sent to
 * \arg \b client-id (int) - Client ID for new client
 * 
 * \li This will be the first message received over the notification band by a newly
 * registered client.</p> \n \n
 */
/*void
OSCSender::send_client_registration(const string& url, int client_id)
{
// FIXME
}*/


/** \page client_osc_namespace
 * <p> \b /om/error - Notification that an error has occurred
 * \arg \b message (string) - Error message (natural language text)
 * 
 * \li This is for notification of errors that aren't a direct response to a
 * user command, ie "unexpected" errors.</p> \n \n
 */
void
OSCSender::send_error(const string& msg)
{
	for (list<ClientRecord*>::const_iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		lo_send((*i)->address(), "/om/error", "s", msg.c_str());
}


/** \page client_osc_namespace
 * <p> \b /om/num_plugins
 * \arg \b num (int) - Number of plugins engine has loaded
 * \li This is sent before sending the list of plugins, so the client is aware
 * of how many plugins (/om/plugin messages) to expect.</p> \n \n
 */


/** \page client_osc_namespace
 * <p> \b /om/plugin - Notification of the existance of a plugin
 * FIXME: update docs
 * \arg \b lib-name (string) - Name of shared library plugin resides in (ie "cmt.so")
 * \arg \b plug-label (string) - Label of the plugin (ie "dahdsr_iaoa")
 * \arg \b name (string) - Descriptive human-readable name of plugin (ie "ADSR Envelope")
 * \arg \b type (string) - Type if plugin ("LADSPA", "DSSI", or "Internal")</p> \n \n
 */
void
OSCSender::send_plugins_to(ClientRecord* client)
{
	om->node_factory()->lock_plugin_list();
	
	const list<Plugin*>& plugs = om->node_factory()->plugins();
	const Plugin* plugin;

	lo_timetag tt;
	lo_timetag_now(&tt);
	lo_bundle b = lo_bundle_new(tt);
	lo_message m = lo_message_new();
	list<lo_message> msgs;

	lo_message_add_int32(m, plugs.size());
	lo_bundle_add_message(b, "/om/num_plugins", m);
	msgs.push_back(m);

	for (list<Plugin*>::const_iterator j = plugs.begin(); j != plugs.end(); ++j) {
		plugin = (*j);
		m = lo_message_new();

		/*lo_message_add_string(m, plugin->lib_name().c_str());
		lo_message_add_string(m, plugin->plug_label().c_str());*/
		lo_message_add_string(m, plugin->type_string());
		lo_message_add_string(m, plugin->uri().c_str());
		lo_message_add_string(m, plugin->plug_label().c_str());
		lo_message_add_string(m, plugin->name().c_str());
		lo_bundle_add_message(b, "/om/plugin", m);
		msgs.push_back(m);
		if (lo_bundle_length(b) > 1024) {
			lo_send_bundle(client->address(), b);
			lo_bundle_free(b);
			b = lo_bundle_new(tt);
			//usleep(100);  // plugins get lost in the ether without this :/  (?) 
		}
	}
	
	if (lo_bundle_length(b) > 0) {
		lo_send_bundle(client->address(), b);
		lo_bundle_free(b);
	} else {
		lo_bundle_free(b);
	}
	for (list<lo_bundle>::const_iterator i = msgs.begin(); i != msgs.end(); ++i)
		lo_message_free(*i);

	om->node_factory()->unlock_plugin_list();
}


void
OSCSender::send_node(const Node* const node)
{
	for (list<ClientRecord*>::const_iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		send_node_to((*i), node);
}


void
OSCSender::send_new_port(const Port* port)
{
	for (list<ClientRecord*>::const_iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		send_new_port_to((*i), port);
}


void
OSCSender::send_destroyed(const string& path)
{
	assert(path != "/");
	for (list<ClientRecord*>::const_iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		send_destroyed_to((*i), path);
}

void
OSCSender::send_patch_cleared(const string& patch_path)
{
	for (list<ClientRecord*>::const_iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		send_patch_cleared_to((*i), patch_path);
}

void
OSCSender::send_connection(const Connection* const c)
{
	for (list<ClientRecord*>::const_iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		send_connection_to((*i), c);
}


void
OSCSender::send_disconnection(const string& src_port_path, const string& dst_port_path)
{
	for (list<ClientRecord*>::const_iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		send_disconnection_to((*i), src_port_path, dst_port_path);
}


void
OSCSender::send_patch_enable(const string& patch_path)
{
	for (list<ClientRecord*>::const_iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		send_patch_enable_to((*i), patch_path);
}


void
OSCSender::send_patch_disable(const string& patch_path)
{
	for (list<ClientRecord*>::const_iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		send_patch_disable_to((*i), patch_path);
}


/** Send notification of a metadata update.
 *
 * Like control changes, does not send update to client that set the metadata, if applicable.
 */
void
OSCSender::send_metadata_update(const string& node_path, const string& key, const string& value)
{
	for (list<ClientRecord*>::const_iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		send_metadata_update_to((*i), node_path, key, value);
}


/** Send notification of a control change.
 *
 * If responder is specified, the notification will not be send to the address of
 * that responder (to avoid sending redundant information back to clients and
 * forcing clients to ignore things to avoid feedback loops etc).
 */
void
OSCSender::send_control_change(const string& port_path, float value)
{
	for (list<ClientRecord*>::const_iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		send_control_change_to((*i), port_path, value);
}


void
OSCSender::send_program_add(const string& node_path, int bank, int program, const string& name)
{
	for (list<ClientRecord*>::const_iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		send_program_add_to((*i), node_path, bank, program, name);
}


void
OSCSender::send_program_remove(const string& node_path, int bank, int program)
{
	for (list<ClientRecord*>::const_iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		send_program_remove_to((*i), node_path, bank, program);
}


/** Send a patch.
 *
 * Sends all objects underneath Patch - contained Nodes, etc.
 */
void
OSCSender::send_patch(const Patch* const p)
{
	for (list<ClientRecord*>::const_iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		send_patch_to((*i), p);
}


/** Sends notification of an OmObject's renaming
 */
void
OSCSender::send_rename(const string& old_path, const string& new_path)
{
	for (list<ClientRecord*>::const_iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		send_rename_to((*i), old_path, new_path);
}


/** Sends all OmObjects known to the engine.
 */
void
OSCSender::send_all_objects()
{
	for (list<ClientRecord*>::const_iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		send_all_objects_to((*i));
}


void
OSCSender::send_node_creation_messages(const Node* const node)
{
	// This is pretty stupid :/  in and out and back again!
	for (list<ClientRecord*>::const_iterator i = m_clients.begin(); i != m_clients.end(); ++i)
		node->send_creation_messages((*i));
}
	

//
//  SINGLE DESTINATION VERSIONS
//


/** \page client_osc_namespace
 * <p> \b /om/new_node - Notification of a new node's creation.
 * \arg \b path (string) - Path of the new node
 * \arg \b polyphonic (integer-boolean) - Node is polyphonic (1 = yes, 0 = no)
 * \arg \b type (string) - Type of plugin (LADSPA, DSSI, Internal, Patch)
 * \arg \b lib-name (string) - Name of library if a plugin (ie cmt.so)
 * \arg \b plug-label (string) - Label of plugin in library (ie adsr_env)
 * 
 * \li New nodes are sent as a bundle.  The first message in the bundle will be
 * this one (/om/new_node), followed by a series of /om/new_port commands,
 * followed by /om/new_node_end. </p> \n \n
 */
void
OSCSender::send_node_to(ClientRecord* client, const Node* const node)
{
	assert(client != NULL);
	
	lo_timetag tt;
	lo_timetag_now(&tt);
	lo_bundle b = lo_bundle_new(tt);
	lo_message m = lo_message_new();
	list<lo_message> msgs;

	lo_message_add_string(m, node->path().c_str());
	lo_message_add_int32(m,
		(node->poly() > 1
		&& node->poly() == node->parent_patch()->internal_poly()
		? 1 : 0));

	lo_message_add_string(m, node->plugin()->type_string());
	lo_message_add_string(m, node->plugin()->uri().c_str());
	/*lo_message_add_string(m, node->plugin()->lib_name().c_str());
	lo_message_add_string(m, node->plugin()->plug_label().c_str());*/
	lo_bundle_add_message(b, "/om/new_node", m);
	msgs.push_back(m);
	
	const Array<Port*>& ports = node->ports();
	Port*     port;
	PortInfo* info;
	for (size_t j=0; j < ports.size(); ++j) {
		port = ports.at(j);
		info = port->port_info();

		assert(port != NULL);
		assert(info != NULL);
		
		m = lo_message_new();
		lo_message_add_string(m, port->path().c_str());
		lo_message_add_string(m, info->type_string().c_str());
		lo_message_add_string(m, info->direction_string().c_str());
		lo_message_add_string(m, info->hint_string().c_str());
		lo_message_add_float(m, info->default_val());
		lo_message_add_float(m, info->min_val());
		lo_message_add_float(m, info->max_val());
		lo_bundle_add_message(b, "/om/new_port", m);
		msgs.push_back(m);
		
		// If the bundle is getting very large, send it and start
		// a new one
		if (lo_bundle_length(b) > 1024) {
		  lo_send_bundle(client->address(), b);
		  lo_bundle_free(b);
		  b = lo_bundle_new(tt);
		}
	}

	m = lo_message_new();
	lo_bundle_add_message(b, "/om/new_node_end", m);
	msgs.push_back(m);

	lo_send_bundle(client->address(), b);
	lo_bundle_free(b);

	for (list<lo_bundle>::const_iterator i = msgs.begin(); i != msgs.end(); ++i)
		lo_message_free(*i);

	usleep(100);

	const map<string, string>& data = node->metadata();
	// Send node metadata
	for (map<string, string>::const_iterator i = data.begin(); i != data.end(); ++i)
		send_metadata_update_to(client, node->path(), (*i).first, (*i).second);

	// Send port metadata
	for (size_t j=0; j < ports.size(); ++j) {
		port = ports.at(j);
		const map<string, string>& data = port->metadata();
		for (map<string, string>::const_iterator i = data.begin(); i != data.end(); ++i)
			send_metadata_update_to(client, port->path(), (*i).first, (*i).second);
	}

	// Send control values
	for (size_t i=0; i < node->ports().size(); ++i) {
		PortBase<sample>* port = (PortBase<sample>*)node->ports().at(i);
		if (port->port_info()->is_input() && port->port_info()->is_control())
			send_control_change_to(client, port->path(), port->buffer(0)->value_at(0));
	}
}



/** \page client_osc_namespace
 * <p> \b /om/new_port - Notification of a new port's creation.
 * \arg \b path (string) - Path of new port
 * \arg \b data-type (string) - Type of port (CONTROL or AUDIO)
 * \arg \b direction (string) - Direction of data flow (INPUT or OUTPUT)
 * \arg \b hint (string) - Hint (INTEGER, LOGARITHMIC, TOGGLE, or NONE)
 * \arg \b default-value (float) - Default (initial) value
 * \arg \b min-value (float) - Suggested minimum value
 * \arg \b min-value (float) - Suggested maximum value
 *
 * \li Note that in the event of loading a patch, this message could be
 * followed immediately by a control change, meaning the default-value is
 * not actually the current value of the port.
 * \li The minimum and maximum values are suggestions only, they are not
 * enforced in any way, and going outside them is perfectly fine.  Also note
 * that the port ranges in om_gtk are not these ones!  Those ranges are set
 * as metadata.</p> \n \n
 */
void
OSCSender::send_new_port_to(ClientRecord* client, const Port* port)
{
	assert(client != NULL);
	
	PortInfo* info = port->port_info();
	
	lo_send(client->address(), "/om/new_port", "ssssfff",
	        port->path().c_str(), info->type_string().c_str(), info->direction_string().c_str(),
			info->hint_string().c_str(), info->default_val(), info->min_val(), info->max_val());
	
	// Send metadata
	const map<string, string>& data = port->metadata();
	for (map<string, string>::const_iterator i = data.begin(); i != data.end(); ++i)
		send_metadata_update_to(client, port->path(), (*i).first, (*i).second);
}


/** \page client_osc_namespace
 * <p> \b /om/destroyed - Notification an object has been destroyed
 * \arg \b path (string) - Path of object (which no longer exists) </p> \n \n
 */
void
OSCSender::send_destroyed_to(ClientRecord* client, const string& path)
{
	assert(client != NULL);
	assert(path != "/");
	
	lo_send(client->address(), "/om/destroyed", "s", path.c_str());
}


/** \page client_osc_namespace
 * <p> \b /om/patch_cleared - Notification a patch has been cleared (all children destroyed)
 * \arg \b path (string) - Path of patch (which is now empty)</p> \n \n
 */
void
OSCSender::send_patch_cleared_to(ClientRecord* client, const string& patch_path)
{
	assert(client != NULL);
	
	lo_send(client->address(), "/om/patch_cleared", "s", patch_path.c_str());
}


/** \page client_osc_namespace
 * <p> \b /om/patch_enabled - Notification a patch's DSP processing has been enabled.
 * \arg \b path (string) - Path of enabled patch</p> \n \n
 */
void
OSCSender::send_patch_enable_to(ClientRecord* client, const string& patch_path)
{
	assert(client != NULL);
	
	lo_send(client->address(), "/om/patch_enabled", "s", patch_path.c_str());
}


/** \page client_osc_namespace
 * <p> \b /om/patch_disabled - Notification a patch's DSP processing has been disabled.
 * \arg \b path (string) - Path of disabled patch</p> \n \n
 */
void
OSCSender::send_patch_disable_to(ClientRecord* client, const string& patch_path)
{
	assert(client != NULL);
	
	lo_send(client->address(), "/om/patch_disabled", "s", patch_path.c_str());
}


/** \page client_osc_namespace
 * <p> \b /om/new_connection - Notification a new connection has been made.
 * \arg \b src-path (string) - Path of the source port
 * \arg \b dst-path (string) - Path of the destination port</p> \n \n
 */
void
OSCSender::send_connection_to(ClientRecord* client, const Connection* const c)
{
	assert(client != NULL);
	
	lo_send(client->address(), "/om/new_connection", "ss", c->src_port()->path().c_str(), c->dst_port()->path().c_str());
}


/** \page client_osc_namespace
 * <p> \b /om/disconnection - Notification a connection has been unmade.
 * \arg \b src-path (string) - Path of the source port
 * \arg \b dst-path (string) - Path of the destination port</p> \n \n
 */
void
OSCSender::send_disconnection_to(ClientRecord* client, const string& src_port_path, const string& dst_port_path)
{
	assert(client != NULL);
	
	lo_send(client->address(), "/om/disconnection", "ss", src_port_path.c_str(), dst_port_path.c_str());
}


/** \page client_osc_namespace
 * <p> \b /om/metadata/update - Notification of a piece of metadata.
 * \arg \b path (string) - Path of the object associated with metadata (can be a node, patch, or port)
 * \arg \b key (string)
 * \arg \b value (string)</p> \n \n
 */
void
OSCSender::send_metadata_update_to(ClientRecord* client, const string& path, const string& key, const string& value)
{
	assert(client != NULL);
	
	lo_send(client->address(), "/om/metadata/update", "sss", path.c_str(), key.c_str(), value.c_str());
}


/** \page client_osc_namespace
 * <p> \b /om/control_change - Notification the value of a port has changed
 * \arg \b path (string) - Path of port
 * \arg \b value (float) - New value of port
 *
 * \li This will only send updates for values set by clients of course - not values
 * changing because of connections to other ports!</p> \n \n
 */
void
OSCSender::send_control_change_to(ClientRecord* client, const string& port_path, float value)
{
	assert(client != NULL);
	
	lo_send(client->address(), "/om/control_change", "sf", port_path.c_str(), value);
}


/** \page client_osc_namespace
 * <p> \b /om/new_patch - Notification of a new patch
 * \arg \b path (string) - Path of new patch
 * \arg \b poly (int) - Polyphony of new patch (\em not a boolean like new_node) </p> \n \n
 */
void
OSCSender::send_patch_to(ClientRecord* client, const Patch* const p)
{
	assert(client != NULL);
	
	lo_send(client->address(), "/om/new_patch", "si", p->path().c_str(), p->internal_poly());
	
	if (p->process())
		send_patch_enable_to(client, p->path());
	
	// Send metadata
	const map<string, string>& data = p->metadata();
	for (map<string, string>::const_iterator i = data.begin(); i != data.end(); ++i) {
		send_metadata_update_to(client, p->path(), (*i).first, (*i).second);
	}
}


/** \page client_osc_namespace
 * <p> \b /om/object_renamed - Notification of an object's renaming
 * \arg \b old-path (string) - Old path of object
 * \arg \b new-path (string) - New path of object </p> \n \n
 */
void
OSCSender::send_rename_to(ClientRecord* client, const string& old_path, const string& new_path)
{
	assert(client != NULL);

	lo_send(client->address(), "/om/object_renamed", "ss", old_path.c_str(), new_path.c_str());
}


/** Sends all OmObjects known to the engine.
 */
void
OSCSender::send_all_objects_to(ClientRecord* client)
{
	assert(client != NULL);
	
	for (Tree<OmObject*>::iterator i = om->object_store()->objects().begin();
			i != om->object_store()->objects().end(); ++i)
		if ((*i)->as_node() != NULL && (*i)->parent() == NULL)
			(*i)->as_node()->send_creation_messages(client);
}


/** Sends information about a program associated with a DSSI plugin node.
 */
void
OSCSender::send_program_add_to(ClientRecord* client, const string& node_path, int bank, int program, const string& name)
{
	assert(client != NULL);
  
	lo_send(client->address(), "/om/program_add", "siis", 
		node_path.c_str(), bank, program, name.c_str());
}


void
OSCSender::send_program_remove_to(ClientRecord* client, const string& node_path, int bank, int program)
{
	assert(client != NULL);
  
	lo_send(client->address(), "/om/program_remove", "sii", 
		node_path.c_str(), bank, program);
}


} // namespace Om
