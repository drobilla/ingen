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

#include "OSCClient.h"
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
#include "Plugin.h"
#include "TypedPort.h"
#include "Connection.h"
#include "AudioDriver.h"
#include "interface/ClientInterface.h"
#include "Responder.h"

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

	
/* Documentation for namespace portion implemented in Responder.cpp */
	
/** \page client_osc_namespace
 * \n
 * <h3>Notification Band</h3>
 */

/** \page client_osc_namespace
 * <p> \b /om/response/ok - Respond successfully to a user command
 * \arg \b responder-id (int) - Responder ID this is a response to
 * </p> \n \n
 */
	
/** \page client_osc_namespace
 * <p> \b /om/response/error - Respond negatively to a user command
 * \arg \b responder-id (int) - Request ID this is a response to
 * \arg \b message (string) - Error message (natural language text)
 * </p> \n \n
 */



/** \page client_osc_namespace
 * \n
 * <h3>Notification Band</h3>
 */

/** \page client_osc_namespace
 * <p> \b /om/error - Notification that an error has occurred
 * \arg \b message (string) - Error message (natural language text)
 * 
 * \li This is for notification of errors that aren't a direct response to a
 * user command, ie "unexpected" errors.</p> \n \n
 */
void
OSCClient::error(const string& msg)
{
	lo_send(_address, "/om/error", "s", msg.c_str());
}


/** \page client_osc_namespace
 * <p> \b /om/num_plugins
 * \arg \b num (int) - Number of plugins engine has loaded
 * \li This is sent before sending the list of plugins, so the client is aware
 * of how many plugins (/om/plugin messages) to expect.</p> \n \n
 */


/** \page client_osc_namespace
 * <p> \b /om/num_plugins
 * \arg \b num (int) - Number of plugins engine has loaded
 * \li This is sent before sending the list of plugins, so the client is aware
 * of how many plugins (/om/plugin messages) to expect.</p> \n \n
 */
void
OSCClient::num_plugins(uint32_t num)
{
	lo_message m = lo_message_new();
	lo_message_add_int32(m, num);
	lo_send_message(_address, "/om/num_plugins", m);
}


/** \page client_osc_namespace
 * <p> \b /om/plugin - Notification of the existance of a plugin
 * \arg \b type (string) - Type if plugin ("LADSPA", "DSSI", or "Internal")
 * \arg \b uri (string) - URI of the plugin (see engine namespace documentation) \n
 * \arg \b lib-name (string) - Name of shared library plugin resides in (ie "cmt.so")
 * \arg \b plug-label (string) - Label of the plugin (ie "dahdsr_iaoa")
 * \arg \b name (string) - Descriptive human-readable name of plugin (ie "ADSR Envelope")
 * </p> \n \n
 */
/*
void
OSCClient::plugins()
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

		lo_message_add_string(m, plugin->type_string());
		lo_message_add_string(m, plugin->uri().c_str());
		lo_message_add_string(m, plugin->plug_label().c_str());
		lo_message_add_string(m, plugin->name().c_str());
		lo_bundle_add_message(b, "/om/plugin", m);
		msgs.push_back(m);
		if (lo_bundle_length(b) > 1024) {
			lo_send_bundle(_address, b);
			lo_bundle_free(b);
			b = lo_bundle_new(tt);
		}
	}
	
	if (lo_bundle_length(b) > 0) {
		lo_send_bundle(_address, b);
		lo_bundle_free(b);
	} else {
		lo_bundle_free(b);
	}
	for (list<lo_bundle>::const_iterator i = msgs.begin(); i != msgs.end(); ++i)
		lo_message_free(*i);

	om->node_factory()->unlock_plugin_list();
}
*/

/** \page client_osc_namespace
 * <p> \b /om/new_node - Notification of a new node's creation.
 * \arg \b plug-uri (string) - URI of the plugin new node is an instance of
 * \arg \b path (string) - Path of the new node
 * \arg \b polyphonic (integer-boolean) - Node is polyphonic (1 = yes, 0 = no)
 * \arg \b num-ports (integer) - Number of ports (number of new_port messages to expect)
 * \li New nodes are sent as a bundle.  The first message in the bundle will be
 * this one (/om/new_node), followed by a series of /om/new_port commands,
 * followed by /om/new_node_end. </p> \n \n
 */
void OSCClient::new_node(const string& plugin_type,
                         const string& plugin_uri,
                         const string& node_path,
                         bool          is_polyphonic,
                         uint32_t      num_ports)
{
	lo_send(_address, "/om/new_node", "sssii", plugin_type.c_str(), plugin_uri.c_str(),
	        node_path.c_str(), is_polyphonic ? 1 : 0, num_ports);
#if 0
	/*
	lo_timetag tt;
	lo_timetag_now(&tt);
	lo_bundle b = lo_bundle_new(tt);
	lo_message m = lo_message_new();
	list<lo_message> msgs;

	lo_message_add_string(m, plugin_type.c_str());
	lo_message_add_string(m, plugin_uri.c_str());
	lo_message_add_string(m, node_path.c_str());
	lo_message_add_int32(m, is_polyphonic ? 1 : 0);
	lo_message_add_int32(m, num_ports);

	lo_bundle_add_message(b, "/om/new_node", m);
	msgs.push_back(m);
*/


	/*
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
		  lo_send_bundle(_address, b);
		  lo_bundle_free(b);
		  b = lo_bundle_new(tt);
		}
	}
*/
	/*m = lo_message_new();
	//lo_bundle_add_message(b, "/om/new_node_end", m);
	//msgs.push_back(m);

	lo_send_bundle(_address, b);
	lo_bundle_free(b);

	for (list<lo_bundle>::const_iterator i = msgs.begin(); i != msgs.end(); ++i)
		lo_message_free(*i);

	usleep(100);
*/
	/*
	const map<string, string>& data = node->metadata();
	// Send node metadata
	for (map<string, string>::const_iterator i = data.begin(); i != data.end(); ++i)
		metadata_update(node->path(), (*i).first, (*i).second);

	
	// Send port metadata
	for (size_t j=0; j < ports.size(); ++j) {
		port = ports.at(j);
		const map<string, string>& data = port->metadata();
		for (map<string, string>::const_iterator i = data.begin(); i != data.end(); ++i)
			metadata_update(port->path(), (*i).first, (*i).second);
	}

	// Send control values
	for (size_t i=0; i < node->ports().size(); ++i) {
		TypedPort<sample>* port = (TypedPort<sample>*)node->ports().at(i);
		if (port->port_info()->is_input() && port->port_info()->is_control())
			control_change(port->path(), port->buffer(0)->value_at(0));
	}
	*/
#endif
}



/** \page client_osc_namespace
 * <p> \b /om/new_port - Notification of a new port's creation.
 * \arg \b path (string) - Path of new port
 * \arg \b data-type (string) - Type of port (CONTROL or AUDIO)
 * \arg \b direction ("is-output") (integer) - Direction of data flow (Input = 0, Output = 1)
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
OSCClient::new_port(const string& path,
	                const string& data_type,
	                bool          is_output)
{
	//PortInfo* info = port->port_info();
	
	lo_send(_address, "/om/new_port", "ssi", path.c_str(), data_type.c_str(), is_output);
	
	// Send metadata
	/*const map<string, string>& data = port->metadata();
	for (map<string, string>::const_iterator i = data.begin(); i != data.end(); ++i)
		metadata_update(port->path(), (*i).first, (*i).second);*/
}


/** \page client_osc_namespace
 * <p> \b /om/destroyed - Notification an object has been destroyed
 * \arg \b path (string) - Path of object (which no longer exists) </p> \n \n
 */
void
OSCClient::object_destroyed(const string& path)
{
	assert(path != "/");
	
	lo_send(_address, "/om/destroyed", "s", path.c_str());
}


/** \page client_osc_namespace
 * <p> \b /om/patch_cleared - Notification a patch has been cleared (all children destroyed)
 * \arg \b path (string) - Path of patch (which is now empty)</p> \n \n
 */
void
OSCClient::patch_cleared(const string& patch_path)
{
	lo_send(_address, "/om/patch_cleared", "s", patch_path.c_str());
}


/** \page client_osc_namespace
 * <p> \b /om/patch_enabled - Notification a patch's DSP processing has been enabled.
 * \arg \b path (string) - Path of enabled patch</p> \n \n
 */
void
OSCClient::patch_enabled(const string& patch_path)
{
	lo_send(_address, "/om/patch_enabled", "s", patch_path.c_str());
}


/** \page client_osc_namespace
 * <p> \b /om/patch_disabled - Notification a patch's DSP processing has been disabled.
 * \arg \b path (string) - Path of disabled patch</p> \n \n
 */
void
OSCClient::patch_disabled(const string& patch_path)
{
	lo_send(_address, "/om/patch_disabled", "s", patch_path.c_str());
}


/** \page client_osc_namespace
 * <p> \b /om/new_connection - Notification a new connection has been made.
 * \arg \b src-path (string) - Path of the source port
 * \arg \b dst-path (string) - Path of the destination port</p> \n \n
 */
void
OSCClient::connection(const string& src_port_path, const string& dst_port_path)
{
	lo_send(_address, "/om/new_connection", "ss", src_port_path.c_str(), dst_port_path.c_str());
}


/** \page client_osc_namespace
 * <p> \b /om/disconnection - Notification a connection has been unmade.
 * \arg \b src-path (string) - Path of the source port
 * \arg \b dst-path (string) - Path of the destination port</p> \n \n
 */
void
OSCClient::disconnection(const string& src_port_path, const string& dst_port_path)
{
	lo_send(_address, "/om/disconnection", "ss", src_port_path.c_str(), dst_port_path.c_str());
}


/** \page client_osc_namespace
 * <p> \b /om/metadata/update - Notification of a piece of metadata.
 * \arg \b path (string) - Path of the object associated with metadata (can be a node, patch, or port)
 * \arg \b key (string)
 * \arg \b value (string)</p> \n \n
 */
void
OSCClient::metadata_update(const string& path, const string& key, const string& value)
{
	lo_send(_address, "/om/metadata/update", "sss", path.c_str(), key.c_str(), value.c_str());
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
OSCClient::control_change(const string& port_path, float value)
{
	lo_send(_address, "/om/control_change", "sf", port_path.c_str(), value);
}


/** \page client_osc_namespace
 * <p> \b /om/plugin - Notification of the existance of a plugin
 * \arg \b type (string) - Type if plugin ("LADSPA", "DSSI", or "Internal")</p> \n \n
 * \arg \b uri (string) - Type if plugin ("LADSPA", "DSSI", or "Internal")</p> \n \n
 * \arg \b name (string) - Descriptive human-readable name of plugin (ie "ADSR Envelope")
 */
void
OSCClient::new_plugin(const string& type, const string& uri, const string& name)
{
	lo_message m = lo_message_new();
	lo_message_add_string(m, type.c_str());
	lo_message_add_string(m, uri.c_str());
	lo_message_add_string(m, name.c_str());
	lo_send_message(_address, "/om/plugin", m);
}


/** \page client_osc_namespace
 * <p> \b /om/new_patch - Notification of a new patch
 * \arg \b path (string) - Path of new patch
 * \arg \b poly (int) - Polyphony of new patch (\em not a boolean like new_node) </p> \n \n
 */
void
OSCClient::new_patch(const string& path, uint32_t poly)
{
	lo_send(_address, "/om/new_patch", "si", path.c_str(), poly);
	
	/*
	if (p->process())
		patch_enabled(p->path());
	
	// Send metadata
	const map<string, string>& data = p->metadata();
	for (map<string, string>::const_iterator i = data.begin(); i != data.end(); ++i) {
		metadata_update(p->path(), (*i).first, (*i).second);
	}
	*/
}


/** \page client_osc_namespace
 * <p> \b /om/object_renamed - Notification of an object's renaming
 * \arg \b old-path (string) - Old path of object
 * \arg \b new-path (string) - New path of object </p> \n \n
 */
void
OSCClient::object_renamed(const string& old_path, const string& new_path)
{
	lo_send(_address, "/om/object_renamed", "ss", old_path.c_str(), new_path.c_str());
}


/** Sends all GraphObjects known to the engine.
 */
/*
void
OSCClient::all_objects()
{
	for (Tree<GraphObject*>::iterator i = om->object_store()->objects().begin();
			i != om->object_store()->objects().end(); ++i)
		if ((*i)->as_node() != NULL && (*i)->parent() == NULL)
			(*i)->as_node()->send_creation_messages(this);
}
*/

/** Sends information about a program associated with a DSSI plugin node.
 */
void
OSCClient::program_add(const string& node_path, uint32_t bank, uint32_t program, const string& name)
{
	lo_send(_address, "/om/program_add", "siis", 
		node_path.c_str(), bank, program, name.c_str());
}


void
OSCClient::program_remove(const string& node_path, uint32_t bank, uint32_t program)
{
	lo_send(_address, "/om/program_remove", "sii", 
		node_path.c_str(), bank, program);
}


} // namespace Om
