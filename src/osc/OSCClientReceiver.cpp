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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "raul/log.hpp"
#include "raul/AtomLiblo.hpp"
#include "raul/Path.hpp"

#include "ingen-config.h"
#include "OSCClientReceiver.hpp"

#define LOG(s) s << "[OSCClientReceiver] "

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {

OSCClientReceiver::OSCClientReceiver(int                        listen_port,
                                     SharedPtr<ClientInterface> target)
	: _target(target)
	, _st(NULL)
	, _listen_port(listen_port)
{
#ifdef RAUL_LOG_DEBUG
	start(true);
#else
	start(false); // true = dump, false = shutup
#endif
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
		LOG(error) << "Could not start OSC listener.  Aborting." << endl;
		exit(EXIT_FAILURE);
	} else {
		LOG(info) << "Started OSC listener on port " << lo_server_thread_get_port(_st) << endl;
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
	printf("[OSCClientReceiver] %s (%s)\t", path, types);

	for (int i=0; i < argc; ++i) {
		lo_arg_pp(lo_type(types[i]), argv[i]);
		printf("\t");
	}
	printf("\n");

	return 1;  // not handled
}

void
OSCClientReceiver::lo_error_cb(int num, const char* msg, const char* path)
{
	LOG(error) << "Got error from server: " << msg << endl;
}

int
OSCClientReceiver::unknown_cb(const char* path, const char* types, lo_arg** argv, int argc, void* data, void* user_data)
{
	std::string msg = "Received unknown OSC message: ";
	msg += path;

	LOG(error) << msg << endl;

	return 0;
}

void
OSCClientReceiver::setup_callbacks()
{
	if (!_target)
		return;

	lo_server_thread_add_method(_st, "/ok", "i", response_ok_cb, this);
	lo_server_thread_add_method(_st, "/error", "is", response_error_cb, this);
	lo_server_thread_add_method(_st, "/plugin", "sss", plugin_cb, this);
	lo_server_thread_add_method(_st, "/put", NULL, put_cb, this);
	lo_server_thread_add_method(_st, "/delta_begin", NULL, delta_begin_cb, this);
	lo_server_thread_add_method(_st, "/delta_remove", NULL, delta_remove_cb, this);
	lo_server_thread_add_method(_st, "/delta_add", NULL, delta_add_cb, this);
	lo_server_thread_add_method(_st, "/delta_end", NULL, delta_end_cb, this);
	lo_server_thread_add_method(_st, "/move", "ss", move_cb, this);
	lo_server_thread_add_method(_st, "/delete", "s", del_cb, this);
	lo_server_thread_add_method(_st, "/connect", "ss", connection_cb, this);
	lo_server_thread_add_method(_st, "/disconnect", "ss", disconnection_cb, this);
	lo_server_thread_add_method(_st, "/set_property", NULL, set_property_cb, this);
	lo_server_thread_add_method(_st, "/activity", "sT", activity_cb, this);
	lo_server_thread_add_method(_st, "/activity", "sf", activity_cb, this);
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
OSCClientReceiver::_put_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* obj_path = &argv[0]->s;
	const char* ctx      = &argv[1]->s;
	Resource::Properties prop;
	for (int i = 2; i < argc-1; i += 2)
		prop.insert(make_pair(&argv[i]->s,
		                      AtomLiblo::lo_arg_to_atom(types[i+1], argv[i+1])));
	_target->put(obj_path, prop, Resource::uri_to_graph(ctx));
	return 0;
}

int
OSCClientReceiver::_delta_begin_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* obj_path = &argv[0]->s;
	assert(_delta_remove.empty());
	assert(_delta_add.empty());
	_delta_uri = obj_path;
	return 0;
}

int
OSCClientReceiver::_delta_remove_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	_delta_remove.insert(make_pair(&argv[0]->s,
	                               AtomLiblo::lo_arg_to_atom(types[1], argv[1])));
	return 0;
}

int
OSCClientReceiver::_delta_add_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	_delta_add.insert(make_pair(&argv[0]->s,
	                            AtomLiblo::lo_arg_to_atom(types[1], argv[1])));
	return 0;
}

int
OSCClientReceiver::_delta_end_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	_target->delta(_delta_uri, _delta_remove, _delta_add);
	_delta_uri = Raul::URI();
	_delta_remove.clear();
	_delta_add.clear();
	return 0;
}

int
OSCClientReceiver::_move_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* old_path = &argv[1]->s;
	const char* new_path = &argv[2]->s;

	_target->move(old_path, new_path);
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
	const char* src_uri = &argv[0]->s;
	const char* dst_uri = &argv[1]->s;

	_target->disconnect(src_uri, dst_uri);

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
OSCClientReceiver::_activity_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* const port_path = &argv[0]->s;

	Atom value = AtomLiblo::lo_arg_to_atom(types[1], argv[1]);

	_target->activity(port_path, value);

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