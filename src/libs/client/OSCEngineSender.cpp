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

#include <iostream>
#include <raul/AtomLiblo.hpp>
#include "OSCEngineSender.hpp"

using namespace std;

namespace Ingen {
namespace Client {

/** Note the sending port is implicitly set by liblo, lo_send by default sends
 * from the most recently created server, so create the OSC listener before
 * this to have it all happen on the same port.  Yeah, this is a big magic :/
 */
OSCEngineSender::OSCEngineSender(const string& engine_url)
: _engine_url(engine_url)
, _engine_addr(lo_address_new_from_url(engine_url.c_str()))
, _id(0)
{
}


OSCEngineSender::~OSCEngineSender()
{
	lo_address_free(_engine_addr);
}


/** Attempt to connect to the engine (by pinging it).
 *
 * This doesn't register a client (or otherwise affect the client/engine state).
 * To check for success wait for the ping response with id @a ping_id (using the
 * relevant OSCClientReceiver).
 *
 * Passing a client_port of 0 will automatically choose a free port.  If the
 * @a block parameter is true, this function will not return until a connection
 * has successfully been made.
 */
void
OSCEngineSender::attach(int32_t ping_id, bool block)
{
	cerr << "FIXME: attach\n";
	//start_listen_thread(_client_port);
	
	/*if (engine_url == "") {
		string local_url = _osc_listener->listen_url().substr(
			0, _osc_listener->listen_url().find_last_of(":"));
		local_url.append(":16180");
		_engine_addr = lo_address_new_from_url(local_url.c_str());
	} else {
		_engine_addr = lo_address_new_from_url(engine_url.c_str());
	}
	*/
	_engine_addr = lo_address_new_from_url(_engine_url.c_str());

	if (_engine_addr == NULL) {
		cerr << "Unable to connect, aborting." << endl;
		exit(EXIT_FAILURE);
	}

	cout << "[OSCEngineSender] Attempting to contact engine at " << _engine_url << " ..." << endl;

	_id = ping_id;
	this->ping();

	/*if (block) {
		set_wait_response_id(request_id);	

		while (1) {	
			if (_response_semaphore.try_wait() != 0) {
				cout << ".";
				cout.flush();
				ping(request_id);
				usleep(100000);
			} else {
				cout << " connected." << endl;
				_waiting_for_response = false;
				break;
			}
		}
	}
	*/
}

/* *** EngineInterface implementation below here *** */


/** Register with the engine via OSC.
 *
 * Note that this does not actually use 'key', since the engine creates
 * it's own key for OSC clients (namely the incoming URL), for NAT
 * traversal.  It is a parameter to remain compatible with EngineInterface.
 */
void
OSCEngineSender::register_client(ClientInterface* client)
{
	// FIXME: use parameters.. er, somehow.
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/register_client", "i", next_id());
}


void
OSCEngineSender::unregister_client(const string& uri)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/unregister_client", "i", next_id());
}



// Engine commands
void
OSCEngineSender::load_plugins()
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/load_plugins", "i", next_id());
}


void
OSCEngineSender::activate()    
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/activate", "i", next_id());
}


void
OSCEngineSender::deactivate()  
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/deactivate", "i", next_id());
}


void
OSCEngineSender::quit()        
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/quit", "i", next_id());
}


		
// Object commands

void
OSCEngineSender::create_patch(const string& path,
                              uint32_t      poly)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/create_patch", "isi",
		next_id(),
		path.c_str(),
		poly);
}


void
OSCEngineSender::create_port(const string& path,
                             const string& data_type,
                             bool          is_output)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/create_port",  "issi",
		next_id(),
		path.c_str(),
		data_type.c_str(),
		(is_output ? 1 : 0));
}


void
OSCEngineSender::create_node(const string& path,
                             const string& plugin_uri,
                             bool          polyphonic)
{
	assert(_engine_addr);

	if (polyphonic)
		lo_send(_engine_addr, "/ingen/create_node",  "issT",
			next_id(),
			path.c_str(),
			plugin_uri.c_str());
	else
		lo_send(_engine_addr, "/ingen/create_node",  "issF",
			next_id(),
			path.c_str(),
			plugin_uri.c_str());
}


/** Create a node using library name and plugin label (DEPRECATED).
 *
 * DO NOT USE THIS.
 */
void
OSCEngineSender::create_node(const string& path,
                             const string& plugin_type,
                             const string& library_name,
                             const string& plugin_label,
                             bool          polyphonic)
{
	assert(_engine_addr);
	if (polyphonic)
		lo_send(_engine_addr, "/ingen/create_node",  "issssT",
			next_id(),
			path.c_str(),
			plugin_type.c_str(),
			library_name.c_str(),
			plugin_label.c_str());
	else
		lo_send(_engine_addr, "/ingen/create_node",  "issssF",
			next_id(),
			path.c_str(),
			plugin_type.c_str(),
			library_name.c_str(),
			plugin_label.c_str());
}


void
OSCEngineSender::rename(const string& old_path,
                        const string& new_name)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/rename", "iss",
		next_id(),
		old_path.c_str(),
		new_name.c_str());
}


void
OSCEngineSender::destroy(const string& path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/destroy", "is",
		next_id(),
		path.c_str());
}


void
OSCEngineSender::clear_patch(const string& patch_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/clear_patch", "is",
		next_id(),
		patch_path.c_str());
}

	
void
OSCEngineSender::set_polyphony(const string& patch_path, uint32_t poly)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/set_polyphony", "isi",
		next_id(),
		patch_path.c_str(),
		poly);
}

	
void
OSCEngineSender::set_polyphonic(const string& path, bool poly)
{
	assert(_engine_addr);
	if (poly) {
		lo_send(_engine_addr, "/ingen/set_polyphonic", "isT",
				next_id(),
				path.c_str());
	} else {
		lo_send(_engine_addr, "/ingen/set_polyphonic", "isF",
				next_id(),
				path.c_str());
	}
}


void
OSCEngineSender::enable_patch(const string& patch_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/enable_patch", "is",
		next_id(),
		patch_path.c_str());
}


void
OSCEngineSender::disable_patch(const string& patch_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/disable_patch", "is",
		next_id(),
		patch_path.c_str());
}


void
OSCEngineSender::connect(const string& src_port_path,
                         const string& dst_port_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/connect", "iss",
		next_id(),
		src_port_path.c_str(),
		dst_port_path.c_str());
}


void
OSCEngineSender::disconnect(const string& src_port_path,
                            const string& dst_port_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/disconnect", "iss",
		next_id(),
		src_port_path.c_str(),
		dst_port_path.c_str());
}


void
OSCEngineSender::disconnect_all(const string& node_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/disconnect_all", "is",
		next_id(),
		node_path.c_str());
}


void
OSCEngineSender::set_port_value(const string& port_path,
                                const string& type_uri,
                                uint32_t      data_size,
                                const void*   data)
{
	assert(_engine_addr);
	if (type_uri == "ingen:control") {
		assert(data_size == 4);
		lo_send(_engine_addr, "/ingen/set_port_value", "isf",
				next_id(),
				port_path.c_str(),
				*(float*)data);
	} else {
		lo_blob b = lo_blob_new(data_size, (void*)data);
		lo_send(_engine_addr, "/ingen/set_port_value", "isb",
				next_id(),
				port_path.c_str(),
				b);
		lo_blob_free(b);
	}
}


void
OSCEngineSender::set_port_value(const string& port_path,
                                const string& type_uri,
                                uint32_t      voice,
                                uint32_t      data_size,
                                const void*   data)
{
	assert(_engine_addr);
	if (type_uri == "ingen:control") {
		assert(data_size == 4);
		lo_send(_engine_addr, "/ingen/set_port_value", "isf",
				next_id(),
				port_path.c_str(),
				*(float*)data);
	} else {
		lo_blob b = lo_blob_new(data_size, data);
		lo_send(_engine_addr, "/ingen/set_port_value", "isb",
				next_id(),
				port_path.c_str(),
				b);
		lo_blob_free(b);
	}
}


void
OSCEngineSender::set_port_value_immediate(const string& port_path,
                                          const string& type_uri,
                                          uint32_t      data_size,
                                          const void*   data)
{
	assert(_engine_addr);

	if (type_uri == "ingen:control") {
		assert(data_size == 4);
		lo_send(_engine_addr, "/ingen/set_port_value_immediate", "isf",
				next_id(),
				port_path.c_str(),
				*(float*)data);
	} else {
		lo_blob b = lo_blob_new(data_size, data);
		lo_send(_engine_addr, "/ingen/set_port_value_immediate", "isb",
				next_id(),
				port_path.c_str(),
				b);
		lo_blob_free(b);
	}
}


void
OSCEngineSender::set_port_value_immediate(const string& port_path,
                                          const string& type_uri,
                                          uint32_t      voice,
                                          uint32_t      data_size,
                                          const void*   data)
{
	assert(_engine_addr);
	
	if (type_uri == "ingen:control") {
		assert(data_size == 4);
		lo_send(_engine_addr, "/ingen/set_port_value_immediate", "isif",
				next_id(),
				port_path.c_str(),
				voice,
				*(float*)data);
	} else {
		lo_blob b = lo_blob_new(data_size, data);
		lo_send(_engine_addr, "/ingen/set_port_value_immediate", "isib",
				next_id(),
				port_path.c_str(),
				voice,
				b);
		lo_blob_free(b);
	}
}


void
OSCEngineSender::set_program(const string& node_path,
                             uint32_t      bank,
                             uint32_t      program)
{
	assert(_engine_addr);
	lo_send(_engine_addr,
			(string("/dssi") + node_path + "/program").c_str(),
			"ii",
			bank,
			program);
}


void
OSCEngineSender::midi_learn(const string& node_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/midi_learn", "is",
		next_id(),
		node_path.c_str());
}


void
OSCEngineSender::set_metadata(const string&     obj_path,
                              const string&     predicate,
                              const Raul::Atom& value)
{
	
	assert(_engine_addr);
	lo_message m = lo_message_new();
	lo_message_add_int32(m, next_id());
	lo_message_add_string(m, obj_path.c_str());
	lo_message_add_string(m, predicate.c_str());
	Raul::AtomLiblo::lo_message_add_atom(m, value);
	lo_send_message(_engine_addr, "/ingen/set_metadata", m);
}


// Requests //

void
OSCEngineSender::ping()
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/ping", "i", next_id());
}


void
OSCEngineSender::request_plugin(const string& uri)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/request_plugin", "is",
		next_id(),
		uri.c_str());
}


void
OSCEngineSender::request_object(const string& path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/request_object", "is",
		next_id(),
		path.c_str());
}


void
OSCEngineSender::request_port_value(const string& port_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/request_port_value", "is",
		next_id(),
		port_path.c_str());
}

void
OSCEngineSender::request_metadata(const string& object_path, const string& key)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/request_metadata", "iss",
		next_id(),
		object_path.c_str(),
		key.c_str());
}


void
OSCEngineSender::request_plugins()
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/request_plugins", "i", next_id());
}


void
OSCEngineSender::request_all_objects()
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/ingen/request_all_objects", "i", next_id());
}



} // namespace Client
} // namespace Ingen


