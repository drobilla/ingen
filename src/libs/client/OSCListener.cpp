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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "OSCListener.h"
//#include "NodeModel.h"
//#include "PluginModel.h"
#include <list>
#include <cassert>
#include <cstring>
#include <iostream>
using std::cerr; using std::cout; using std::endl;

namespace LibOmClient {

	
/** Construct a OSCListener with a user-provided ModelClientInterface object for notification
 *  of engine events.
 */
OSCListener::OSCListener(int listen_port)
: _listen_port(listen_port),
  _st(NULL)//,
//  _receiving_node(false),
//  _receiving_node_model(NULL),
//  _receiving_node_num_ports(0),
//  _num_received_ports(0)
{
	start();
}


OSCListener::~OSCListener()
{
	stop();
}


void
OSCListener::start()
{
	if (_st != NULL)
		return;

	if (_listen_port == 0) { 
		_st = lo_server_thread_new(NULL, error_cb);
		_listen_port = lo_server_thread_get_port(_st);
	} else {
		char port_str[8];
		snprintf(port_str, 8, "%d", _listen_port);
		_st = lo_server_thread_new(port_str, error_cb);
	}

	if (_st == NULL) {
		cerr << "[OSCListener] Could not start OSC listener.  Aborting." << endl;
		exit(EXIT_FAILURE);
	} else {
		cout << "[OSCListener] Started OSC listener on port " << lo_server_thread_get_port(_st) << endl;
	}

	// FIXME
	lo_server_thread_add_method(_st, NULL, NULL, generic_cb, NULL);

	//lo_server_thread_add_method(_st, "/om/response/ok", "i", om_response_ok_cb, this);
	//lo_server_thread_add_method(_st, "/om/response/error", "is", om_responseerror_cb, this);
	
	setup_callbacks();

	// Display any uncaught messages to the console
	//lo_server_thread_add_method(_st, NULL, NULL, unknown_cb, NULL);

	lo_server_thread_start(_st);
}


void
OSCListener::stop()
{
	if (_st != NULL) {
		//unregister_client();
		lo_server_thread_free(_st);
		_st = NULL;
	}
}


int
OSCListener::generic_cb(const char* path, const char* types, lo_arg** argv, int argc, void* data, void* user_data)
{
	printf("[OSCMsg] %s (%s)\t", path, types);
	
	for (int i=0; i < argc; ++i) {
		lo_arg_pp(lo_type(types[i]), argv[i]);
		printf("\t");
    }
    printf("\n");
    
	/*for (int i=0; i < argc; ++i) {
		printf("         '%c'  ", types[i]);
		lo_arg_pp(lo_type(types[i]), argv[i]);
		printf("\n");
    }
    printf("\n");*/

	return 1;  // not handled
}


void
OSCListener::error_cb(int num, const char* msg, const char* path)
{
	cerr << "Got error from server: " << msg << endl;
}



int
OSCListener::unknown_cb(const char* path, const char* types, lo_arg** argv, int argc, void* data, void* user_data)
{
	string msg = "Received unknown OSC message: ";
	msg += path;

	cerr << msg << endl;

	return 0;
}


void
OSCListener::setup_callbacks()
{
	lo_server_thread_add_method(_st, "/om/num_plugins", "i", num_plugins_cb, this);
	lo_server_thread_add_method(_st, "/om/plugin", "sss", plugin_cb, this);
	lo_server_thread_add_method(_st, "/om/new_patch", "si", new_patch_cb, this);
	lo_server_thread_add_method(_st, "/om/destroyed", "s", destroyed_cb, this);
	lo_server_thread_add_method(_st, "/om/patch_enabled", "s", patch_enabled_cb, this);
	lo_server_thread_add_method(_st, "/om/patch_disabled", "s", patch_disabled_cb, this);
	lo_server_thread_add_method(_st, "/om/patch_cleared", "s", patch_cleared_cb, this);
	lo_server_thread_add_method(_st, "/om/object_renamed", "ss", object_renamed_cb, this);
	lo_server_thread_add_method(_st, "/om/new_connection", "ss", connection_cb, this);
	lo_server_thread_add_method(_st, "/om/disconnection", "ss", disconnection_cb, this);
	lo_server_thread_add_method(_st, "/om/new_node", "sssii", new_node_cb, this);
	lo_server_thread_add_method(_st, "/om/new_port", "ssi", new_port_cb, this);
	lo_server_thread_add_method(_st, "/om/metadata/update", "sss", metadata_update_cb, this);
	lo_server_thread_add_method(_st, "/om/control_change", "sf", control_change_cb, this);
	lo_server_thread_add_method(_st, "/om/program_add", "siis", program_add_cb, this);
	lo_server_thread_add_method(_st, "/om/program_remove", "sii", program_remove_cb, this);
}


/** Catches errors that aren't a direct result of a client request.
 */
int
OSCListener::m_error_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	cerr << "ERROR: " << argv[0]->s << endl;
	// FIXME
	//error((char*)argv[0]);
	return 0;
}
 

int
OSCListener::m_new_patch_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	new_patch(&argv[0]->s, argv[1]->i); // path, poly
	return 0;
}


int
OSCListener::m_destroyed_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	object_destroyed((const char*)&argv[0]->s);
	return 0;
}


int
OSCListener::m_patch_enabled_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	patch_enabled((const char*)&argv[0]->s);
	return 0;
}


int
OSCListener::m_patch_disabled_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	patch_disabled((const char*)&argv[0]->s);
	return 0;
}


int
OSCListener::m_patch_cleared_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	patch_cleared((const char*)&argv[0]->s);
	return 0;
}


int
OSCListener::m_object_renamed_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	object_renamed((const char*)&argv[0]->s, (const char*)&argv[1]->s);
	return 0;
}


int
OSCListener::m_connection_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* const src_port_path = &argv[0]->s;
	const char* const dst_port_path = &argv[1]->s;
	
	connection(src_port_path, dst_port_path);

	return 0;
}


int
OSCListener::m_disconnection_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* src_port_path = &argv[0]->s;
	const char* dst_port_path = &argv[1]->s;

	disconnection(src_port_path, dst_port_path);

	return 0;
}


/** Notification of a new node creation.
 */
int
OSCListener::m_new_node_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char*   type       = &argv[0]->s;
	const char*   uri        = &argv[1]->s;
	const char*   node_path  = &argv[2]->s;
	const int32_t poly       =  argv[3]->i;
	const int32_t num_ports  =  argv[4]->i;

	new_node(type, uri, node_path, poly, num_ports);

	/*_receiving_node_model = new NodeModel(node_path);
	_receiving_node_model->polyphonic((poly == 1));
	_receiving_node_num_ports = num_ports;

	PluginModel* pi = new PluginModel(type, uri);
	_receiving_node_model->plugin(pi);
	
	_receiving_node = true;
	_num_received_ports = 0;
	*/
	return 0;
}


/** Notification of a new port creation.
 */
int
OSCListener::m_new_port_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* port_path   = &argv[0]->s;
	const char* type        = &argv[1]->s;
	bool        is_output   = (argv[2]->i == 1);
	/*const char* direction   = &argv[2]->s;
	const char* hint        = &argv[3]->s;
	float       default_val =  argv[4]->f;
	float       min_val     =  argv[5]->f;
	float       max_val     =  argv[6]->f;*/

	new_port(port_path, type, is_output);
#if 0
	PortModel::Type ptype = PortModel::CONTROL;
	if (!strcmp(type, "AUDIO")) ptype = PortModel::AUDIO;
	else if (!strcmp(type, "CONTROL")) ptype = PortModel::CONTROL;
	else if (!strcmp(type, "MIDI")) ptype = PortModel::MIDI;
	else cerr << "[OSCListener] WARNING:  Unknown port type received (" << type << ")" << endl;

#if 0
	PortModel::Direction pdir = PortModel::INPUT;
	if (!strcmp(direction, "INPUT")) pdir = PortModel::INPUT;
	else if (!strcmp(direction, "OUTPUT")) pdir = PortModel::OUTPUT;
	else cerr << "[OSCListener] WARNING:  Unknown port direction received (" << direction << ")" << endl;
#endif
	PortModel::Direction pdir = is_output ? PortModel::OUTPUT : PortModel::INPUT;

/*
	PortModel::Hint phint = PortModel::NONE;
	if (!strcmp(hint, "LOGARITHMIC")) phint = PortModel::LOGARITHMIC;
	else if (!strcmp(hint, "INTEGER")) phint = PortModel::INTEGER;
	else if (!strcmp(hint, "TOGGLE")) phint = PortModel::TOGGLE;
	
	PortModel* port_model = new PortModel(port_path, ptype, pdir, phint, default_val, min_val, max_val);
*/	
	PortModel* port_model = new PortModel(port_path, ptype, pdir);
	if (m_receiving_node) {
		assert(m_receiving_node_model);
		m_receiving_node_model->add_port(port_model);
		++m_num_received_ports;
		
		// If transmission is done, send new node to client
		if (m_num_received_ports == m_receiving_node_num_ports) {
			new_node_model(m_receiving_node_model);
			m_receiving_node = false;
			m_receiving_node_model = NULL;
			m_num_received_ports = 0;
		}
	} else {
		new_port_model(port_model);
	}

#endif
	return 0;	
}


/** Notification of a new or updated piece of metadata.
 */
int
OSCListener::m_metadata_update_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* obj_path  = &argv[0]->s;
	const char* key       = &argv[1]->s;
	const char* value     = &argv[2]->s;

	metadata_update(obj_path, key, value);

	return 0;	
}


int
OSCListener::m_control_change_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* const port_path  = &argv[0]->s;
	const float       value      =  argv[1]->f;

	control_change(port_path, value);

	return 0;	
}


/** Number of plugins in engine, should precede /om/plugin messages in response
 * to a /om/send_plugins
 */
int
OSCListener::m_num_plugins_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	/** Not worth it implementing a ModelClientInterface callback for this (?)
	 * Or I'm just lazy.  FIXME? */
	num_plugins(argv[0]->i);

	return 0;
}


/** A plugin info response from the server, in response to a /send_plugins
 */
int
OSCListener::m_plugin_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	assert(argc == 3 && !strcmp(types, "sss"));
	new_plugin(&argv[0]->s, &argv[1]->s, &argv[2]->s); // type, uri

	return 0;	
}


int
OSCListener::m_program_add_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* node_path  = &argv[0]->s;
	int32_t     bank       =  argv[1]->i;
	int32_t     program    =  argv[2]->i;
	const char* name       = &argv[3]->s;

	program_add(node_path, bank, program, name);

	return 0;	
}


int
OSCListener::m_program_remove_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* node_path  = &argv[0]->s;
	int32_t     bank       =  argv[1]->i;
	int32_t     program    =  argv[2]->i;

	program_remove(node_path, bank, program);

	return 0;	
}


} // namespace LibOmClient
