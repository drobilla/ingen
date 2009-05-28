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

#include "OSCClientReceiver.hpp"
#include "raul/AtomLiblo.hpp"
#include <list>
#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {

using namespace Shared;

OSCClientReceiver::OSCClientReceiver(int listen_port, SharedPtr<Shared::ClientInterface> target)
	: _target(target)
	, _listen_port(listen_port)
	, _st(NULL)
{
	start(true); // true = dump, false = shutup
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
	lo_server_thread_add_method(_st, "/ingen/plugin", "sss", plugin_cb, this);
	lo_server_thread_add_method(_st, "/ingen/put", NULL, put_cb, this);
	lo_server_thread_add_method(_st, "/ingen/move", "ss", move_cb, this);
	lo_server_thread_add_method(_st, "/ingen/delete", "s", del_cb, this);
	lo_server_thread_add_method(_st, "/ingen/clear_patch", "s", clear_patch_cb, this);
	lo_server_thread_add_method(_st, "/ingen/new_connection", "ss", connection_cb, this);
	lo_server_thread_add_method(_st, "/ingen/disconnection", "ss", disconnection_cb, this);
	lo_server_thread_add_method(_st, "/ingen/new_port", "sisi", new_port_cb, this);
	lo_server_thread_add_method(_st, "/ingen/set_property", NULL, set_property_cb, this);
	lo_server_thread_add_method(_st, "/ingen/set_port_value", "sf", set_port_value_cb, this);
	lo_server_thread_add_method(_st, "/ingen/set_voice_value", "sif", set_voice_value_cb, this);
	lo_server_thread_add_method(_st, "/ingen/activity", "s", activity_cb, this);
}


/** Catches errors that aren't a direct result of a client request.
 */
int
OSCClientReceiver::_error_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	_target->error((char*)argv[0]);
	return 0;
}


int
OSCClientReceiver::_del_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	_target->del((const char*)&argv[0]->s);
	return 0;
}


int
OSCClientReceiver::_clear_patch_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	_target->clear_patch((const char*)&argv[0]->s);
	return 0;
}


int
OSCClientReceiver::_put_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* obj_path = &argv[0]->s;
	Resource::Properties prop;
	for (int i = 1; i < argc-1; i += 2)
		prop.insert(make_pair(&argv[i]->s, AtomLiblo::lo_arg_to_atom(types[i+1], argv[i+1])));
	_target->put(obj_path, prop);
	return 0;
}


int
OSCClientReceiver::_move_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	_target->move((const char*)&argv[0]->s, (const char*)&argv[1]->s);
	return 0;
}


int
OSCClientReceiver::_connection_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* const src_port_path = &argv[0]->s;
	const char* const dst_port_path = &argv[1]->s;

	_target->connect(src_port_path, dst_port_path);

	return 0;
}


int
OSCClientReceiver::_disconnection_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* src_port_path = &argv[0]->s;
	const char* dst_port_path = &argv[1]->s;

	_target->disconnect(src_port_path, dst_port_path);

	return 0;
}


/** Notification of a new or updated property.
 */
int
OSCClientReceiver::_set_property_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	if (argc != 3 || types[0] != 's' || types[1] != 's')
		return 1;

	const char* obj_uri  = &argv[0]->s;
	const char* key      = &argv[1]->s;

	Atom value = AtomLiblo::lo_arg_to_atom(types[2], argv[2]);

	_target->set_property(obj_uri, key, value);

	return 0;
}


int
OSCClientReceiver::_set_port_value_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* const port_path  = &argv[0]->s;
	const float       value      =  argv[1]->f;

	_target->set_port_value(port_path, value);

	return 0;
}


int
OSCClientReceiver::_set_voice_value_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* const port_path  = &argv[0]->s;
	const int         voice      =  argv[1]->i;
	const float       value      =  argv[2]->f;

	_target->set_voice_value(port_path, voice, value);

	return 0;
}


int
OSCClientReceiver::_activity_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* const port_path = &argv[0]->s;

	_target->activity(port_path);

	return 0;
}


int
OSCClientReceiver::_response_ok_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	assert(!strcmp(types, "i"));
	_target->response_ok(argv[0]->i);

	return 0;
}


int
OSCClientReceiver::_response_error_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	assert(!strcmp(types, "is"));
	_target->response_error(argv[0]->i, &argv[1]->s);

	return 0;
}


} // namespace Client
} // namespace Ingen
