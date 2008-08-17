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

#include "OSCClientReceiver.hpp"
#include <raul/AtomLiblo.hpp>
#include <list>
#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {

	
OSCClientReceiver::OSCClientReceiver(int listen_port)
	: _listen_port(listen_port)
	, _st(NULL)
{
	start(false); // true = dump, false = shutup
}


OSCClientReceiver::~OSCClientReceiver()
{
	stop();
}


void
OSCClientReceiver::start(bool dump_osc)
{
	if (_st != NULL)
		return;

	// Attempt preferred port
	if (_listen_port != 0) {
		char port_str[8];
		snprintf(port_str, 8, "%d", _listen_port);
		_st = lo_server_thread_new(port_str, lo_error_cb);
	}

	// Find a free port
	if (!_st) { 
		_st = lo_server_thread_new(NULL, lo_error_cb);
		_listen_port = lo_server_thread_get_port(_st);
	}

	if (_st == NULL) {
		cerr << "[OSCClientReceiver] Could not start OSC listener.  Aborting." << endl;
		exit(EXIT_FAILURE);
	} else {
		cout << "[OSCClientReceiver] Started OSC listener on port " << lo_server_thread_get_port(_st) << endl;
	}

	// Print all incoming messages
	if (dump_osc)
		lo_server_thread_add_method(_st, NULL, NULL, generic_cb, NULL);

	setup_callbacks();

	// Display any uncaught messages to the console
	//lo_server_thread_add_method(_st, NULL, NULL, unknown_cb, NULL);

	lo_server_thread_start(_st);
}


void
OSCClientReceiver::stop()
{
	if (_st != NULL) {
		//unregister_client();
		lo_server_thread_free(_st);
		_st = NULL;
	}
}


int
OSCClientReceiver::generic_cb(const char* path, const char* types, lo_arg** argv, int argc, void* data, void* user_data)
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
OSCClientReceiver::lo_error_cb(int num, const char* msg, const char* path)
{
	cerr << "Got error from server: " << msg << endl;
}



int
OSCClientReceiver::unknown_cb(const char* path, const char* types, lo_arg** argv, int argc, void* data, void* user_data)
{
	std::string msg = "Received unknown OSC message: ";
	msg += path;

	cerr << msg << endl;

	return 0;
}


void
OSCClientReceiver::setup_callbacks()
{
	lo_server_thread_add_method(_st, "/ingen/ok", "i", response_ok_cb, this);
	lo_server_thread_add_method(_st, "/ingen/error", "is", response_error_cb, this);
	lo_server_thread_add_method(_st, "/ingen/num_plugins", "i", num_plugins_cb, this);
	lo_server_thread_add_method(_st, "/ingen/plugin", "ssss", plugin_cb, this);
	lo_server_thread_add_method(_st, "/ingen/new_patch", "si", new_patch_cb, this);
	lo_server_thread_add_method(_st, "/ingen/destroyed", "s", destroyed_cb, this);
	lo_server_thread_add_method(_st, "/ingen/patch_polyphony", "si", patch_polyphony_cb, this);
	lo_server_thread_add_method(_st, "/ingen/patch_cleared", "s", patch_cleared_cb, this);
	lo_server_thread_add_method(_st, "/ingen/object_renamed", "ss", object_renamed_cb, this);
	lo_server_thread_add_method(_st, "/ingen/new_connection", "ss", connection_cb, this);
	lo_server_thread_add_method(_st, "/ingen/disconnection", "ss", disconnection_cb, this);
	lo_server_thread_add_method(_st, "/ingen/new_node", "ssT", new_node_cb, this);
	lo_server_thread_add_method(_st, "/ingen/new_node", "ssF", new_node_cb, this);
	lo_server_thread_add_method(_st, "/ingen/new_port", "sisi", new_port_cb, this);
	lo_server_thread_add_method(_st, "/ingen/polyphonic", "sT", polyphonic_cb, this);
	lo_server_thread_add_method(_st, "/ingen/polyphonic", "sF", polyphonic_cb, this);
	lo_server_thread_add_method(_st, "/ingen/set_variable", NULL, set_variable_cb, this);
	lo_server_thread_add_method(_st, "/ingen/set_property", NULL, set_property_cb, this);
	lo_server_thread_add_method(_st, "/ingen/set_port_value", "sf", set_port_value_cb, this);
	lo_server_thread_add_method(_st, "/ingen/set_voice_value", "sif", set_voice_value_cb, this);
	lo_server_thread_add_method(_st, "/ingen/port_activity", "s", port_activity_cb, this);
	lo_server_thread_add_method(_st, "/ingen/program_add", "siis", program_add_cb, this);
	lo_server_thread_add_method(_st, "/ingen/program_remove", "sii", program_remove_cb, this);
}


/** Catches errors that aren't a direct result of a client request.
 */
int
OSCClientReceiver::_error_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	error((char*)argv[0]);
	return 0;
}
 

int
OSCClientReceiver::_new_patch_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	new_patch(&argv[0]->s, argv[1]->i); // path, poly
	return 0;
}


int
OSCClientReceiver::_destroyed_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	object_destroyed((const char*)&argv[0]->s);
	return 0;
}


int
OSCClientReceiver::_patch_polyphony_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	patch_polyphony((const char*)&argv[0]->s, argv[1]->i);
	return 0;
}


int
OSCClientReceiver::_patch_cleared_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	patch_cleared((const char*)&argv[0]->s);
	return 0;
}


int
OSCClientReceiver::_object_renamed_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	object_renamed((const char*)&argv[0]->s, (const char*)&argv[1]->s);
	return 0;
}


int
OSCClientReceiver::_connection_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* const src_port_path = &argv[0]->s;
	const char* const dst_port_path = &argv[1]->s;
	
	connect(src_port_path, dst_port_path);

	return 0;
}


int
OSCClientReceiver::_disconnection_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* src_port_path = &argv[0]->s;
	const char* dst_port_path = &argv[1]->s;

	disconnect(src_port_path, dst_port_path);

	return 0;
}


/** Notification of a new node creation.
 */
int
OSCClientReceiver::_new_node_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char*   uri        = &argv[0]->s;
	const char*   node_path  = &argv[1]->s;
	const bool    polyphonic = (types[2] == 'T');

	new_node(uri, node_path, polyphonic);

	return 0;
}


/** Notification of a new port creation.
 */
int
OSCClientReceiver::_new_port_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char*    port_path = &argv[0]->s;
	const uint32_t index     =  argv[1]->i;
	const char*    type      = &argv[2]->s;
	const bool     is_output = (argv[3]->i == 1);

	new_port(port_path, index, type, is_output);
	
	return 0;	
}


/** Notification of an object's polyphonic flag
 */
int
OSCClientReceiver::_polyphonic_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* obj_path = &argv[0]->s;
	const bool  poly     = (types[1] == 'T');

	polyphonic(obj_path, poly);

	return 0;	
}


/** Notification of a new or updated variable.
 */
int
OSCClientReceiver::_set_variable_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	if (argc != 3 || types[0] != 's' || types[1] != 's')
		return 1;

	const char* obj_path  = &argv[0]->s;
	const char* key       = &argv[1]->s;

	Atom value = AtomLiblo::lo_arg_to_atom(types[2], argv[2]);

	set_variable(obj_path, key, value);

	return 0;	
}


/** Notification of a new or updated property.
 */
int
OSCClientReceiver::_set_property_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	if (argc != 3 || types[0] != 's' || types[1] != 's')
		return 1;

	const char* obj_path  = &argv[0]->s;
	const char* key       = &argv[1]->s;

	Atom value = AtomLiblo::lo_arg_to_atom(types[2], argv[2]);

	set_property(obj_path, key, value);

	return 0;	
}


int
OSCClientReceiver::_set_port_value_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* const port_path  = &argv[0]->s;
	const float       value      =  argv[1]->f;

	set_port_value(port_path, value);

	return 0;	
}

	
int
OSCClientReceiver::_set_voice_value_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* const port_path  = &argv[0]->s;
	const int         voice      =  argv[1]->i;
	const float       value      =  argv[2]->f;

	set_voice_value(port_path, voice, value);

	return 0;	
}


int
OSCClientReceiver::_port_activity_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* const port_path = &argv[0]->s;

	port_activity(port_path);

	return 0;	
}


int
OSCClientReceiver::_response_ok_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	assert(!strcmp(types, "i"));
	response_ok(argv[0]->i);

	return 0;
}


int
OSCClientReceiver::_response_error_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	assert(!strcmp(types, "is"));
	response_error(argv[0]->i, &argv[1]->s);

	return 0;
}


/** Number of plugins in engine, should precede /ingen/plugin messages in response
 * to a /ingen/send_plugins
 */
int
OSCClientReceiver::_num_plugins_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	num_plugins(argv[0]->i);

	return 0;
}


/** A plugin info response from the server, in response to an /ingen/send_plugins
 */
int
OSCClientReceiver::_plugin_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	assert(argc == 4 && !strcmp(types, "ssss"));
	new_plugin(&argv[0]->s, &argv[1]->s, &argv[2]->s, &argv[3]->s); // uri, type, symbol, name

	return 0;	
}


int
OSCClientReceiver::_program_add_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* node_path  = &argv[0]->s;
	int32_t     bank       =  argv[1]->i;
	int32_t     program    =  argv[2]->i;
	const char* name       = &argv[3]->s;

	program_add(node_path, bank, program, name);

	return 0;	
}


int
OSCClientReceiver::_program_remove_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* node_path  = &argv[0]->s;
	int32_t     bank       =  argv[1]->i;
	int32_t     program    =  argv[2]->i;

	program_remove(node_path, bank, program);

	return 0;	
}


} // namespace Client
} // namespace Ingen
