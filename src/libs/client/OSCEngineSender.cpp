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
	, _id(0)
{
	_address = lo_address_new_from_url(engine_url.c_str());
}


OSCEngineSender::~OSCEngineSender()
{
	lo_address_free(_address);
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
	if (!_address)
		_address = lo_address_new_from_url(_engine_url.c_str());

	if (_address == NULL) {
		cerr << "Aborting: Unable to connect to " << _engine_url << endl;
		exit(EXIT_FAILURE);
	}

	cout << "[OSCEngineSender] Attempting to contact engine at " << _engine_url << " ..." << endl;

	_id = ping_id;
	this->ping();
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
	send("/ingen/register_client", "i", next_id(), LO_ARGS_END, LO_ARGS_END);
}


void
OSCEngineSender::unregister_client(const string& uri)
{
	send("/ingen/unregister_client", "i", next_id(), LO_ARGS_END);
}


// Engine commands
void
OSCEngineSender::load_plugins()
{
	send("/ingen/load_plugins", "i", next_id(), LO_ARGS_END);
}


void
OSCEngineSender::activate()    
{
	send("/ingen/activate", "i", next_id(), LO_ARGS_END);
}


void
OSCEngineSender::deactivate()  
{
	send("/ingen/deactivate", "i", next_id(), LO_ARGS_END);
}


void
OSCEngineSender::quit()        
{
	send("/ingen/quit", "i", next_id(), LO_ARGS_END);
}


		
// Object commands

void
OSCEngineSender::new_patch(const string& path,
                           uint32_t      poly)
{
	send("/ingen/new_patch", "isi",
		next_id(),
		path.c_str(),
		poly,
		LO_ARGS_END);
}


void
OSCEngineSender::new_port(const string& path,
                          uint32_t      index,
                          const string& data_type,
                          bool          is_output)
{
	// FIXME: use index
	send("/ingen/new_port",  "issi",
		next_id(),
		path.c_str(),
		data_type.c_str(),
		(is_output ? 1 : 0),
		LO_ARGS_END);
}


void
OSCEngineSender::new_node(const string& path,
                          const string& plugin_uri,
                          bool          polyphonic)
{

	if (polyphonic)
		send("/ingen/new_node",  "issT",
			next_id(),
			path.c_str(),
			plugin_uri.c_str(),
			LO_ARGS_END);
	else
		send("/ingen/new_node",  "issF",
			next_id(),
			path.c_str(),
			plugin_uri.c_str(),
			LO_ARGS_END);
}


/** Create a node using library name and plugin label (DEPRECATED).
 *
 * DO NOT USE THIS.
 */
void
OSCEngineSender::new_node_deprecated(const string& path,
                                     const string& plugin_type,
                                     const string& library_name,
                                     const string& plugin_label,
                                     bool          polyphonic)
{
	if (polyphonic)
		send("/ingen/new_node",  "issssT",
			next_id(),
			path.c_str(),
			plugin_type.c_str(),
			library_name.c_str(),
			plugin_label.c_str(),
			LO_ARGS_END);
	else
		send("/ingen/new_node",  "issssF",
			next_id(),
			path.c_str(),
			plugin_type.c_str(),
			library_name.c_str(),
			plugin_label.c_str(),
			LO_ARGS_END);
}


void
OSCEngineSender::rename(const string& old_path,
                        const string& new_name)
{
	send("/ingen/rename", "iss",
		next_id(),
		old_path.c_str(),
		new_name.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::destroy(const string& path)
{
	send("/ingen/destroy", "is",
		next_id(),
		path.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::clear_patch(const string& patch_path)
{
	send("/ingen/clear_patch", "is",
		next_id(),
		patch_path.c_str(),
		LO_ARGS_END);
}

	
void
OSCEngineSender::set_polyphony(const string& patch_path, uint32_t poly)
{
	send("/ingen/set_polyphony", "isi",
		next_id(),
		patch_path.c_str(),
		poly,
		LO_ARGS_END);
}

	
void
OSCEngineSender::set_polyphonic(const string& path, bool poly)
{
	if (poly) {
		send("/ingen/set_polyphonic", "isT",
				next_id(),
				path.c_str(),
				LO_ARGS_END);
	} else {
		send("/ingen/set_polyphonic", "isF",
				next_id(),
				path.c_str(),
				LO_ARGS_END);
	}
}


void
OSCEngineSender::connect(const string& src_port_path,
                         const string& dst_port_path)
{
	send("/ingen/connect", "iss",
		next_id(),
		src_port_path.c_str(),
		dst_port_path.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::disconnect(const string& src_port_path,
                            const string& dst_port_path)
{
	send("/ingen/disconnect", "iss",
		next_id(),
		src_port_path.c_str(),
		dst_port_path.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::disconnect_all(const string& parent_patch_path,
                                const string& node_path)
{
	send("/ingen/disconnect_all", "iss",
		next_id(),
		parent_patch_path.c_str(),
		node_path.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::set_port_value(const string&     port_path,
                                const Raul::Atom& value)
{
	lo_message m = lo_message_new();
	lo_message_add_string(m, port_path.c_str());
	Raul::AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_port_value", m);
}


void
OSCEngineSender::set_voice_value(const string&     port_path,
                                 uint32_t          voice,
                                 const Raul::Atom& value)
{
	lo_message m = lo_message_new();
	lo_message_add_string(m, port_path.c_str());
	lo_message_add_int32(m, voice);
	Raul::AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_port_value", m);
}


void
OSCEngineSender::set_port_value_immediate(const string&     port_path,
                                          const Raul::Atom& value)
{
	lo_message m = lo_message_new();
	lo_message_add_string(m, port_path.c_str());
	Raul::AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_port_value_immediate", m);
}


void
OSCEngineSender::set_voice_value_immediate(const string&     port_path,
                                           uint32_t          voice,
                                           const Raul::Atom& value)
{
	lo_message m = lo_message_new();
	lo_message_add_string(m, port_path.c_str());
	lo_message_add_int32(m, voice);
	Raul::AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_port_value_immediate", m);
}

	
void
OSCEngineSender::enable_port_broadcasting(const string& port_path)
{
	send("/ingen/enable_port_broadcasting", "is",
			next_id(),
			port_path.c_str(),
			LO_ARGS_END);
}


void
OSCEngineSender::disable_port_broadcasting(const string& port_path)
{
	send("/ingen/disable_port_broadcasting", "is",
			next_id(),
			port_path.c_str(),
			LO_ARGS_END);
}


void
OSCEngineSender::set_program(const string& node_path,
                             uint32_t      bank,
                             uint32_t      program)
{
	send((string("/dssi") + node_path + "/program").c_str(),
			"ii",
			bank,
			program,
			LO_ARGS_END);
}


void
OSCEngineSender::midi_learn(const string& node_path)
{
	send("/ingen/midi_learn", "is",
		next_id(),
		node_path.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::set_variable(const string&     obj_path,
                              const string&     predicate,
                              const Raul::Atom& value)
{
	lo_message m = lo_message_new();
	lo_message_add_int32(m, next_id());
	lo_message_add_string(m, obj_path.c_str());
	lo_message_add_string(m, predicate.c_str());
	Raul::AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_variable", m);
}

	
void
OSCEngineSender::set_property(const string&     obj_path,
                              const string&     predicate,
                              const Raul::Atom& value)
{
	lo_message m = lo_message_new();
	lo_message_add_int32(m, next_id());
	lo_message_add_string(m, obj_path.c_str());
	lo_message_add_string(m, predicate.c_str());
	Raul::AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_property", m);
}



// Requests //

void
OSCEngineSender::ping()
{
	send("/ingen/ping", "i", next_id(), LO_ARGS_END);
}


void
OSCEngineSender::request_plugin(const string& uri)
{
	send("/ingen/request_plugin", "is",
		next_id(),
		uri.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::request_object(const string& path)
{
	send("/ingen/request_object", "is",
		next_id(),
		path.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::request_port_value(const string& port_path)
{
	send("/ingen/request_port_value", "is",
		next_id(),
		port_path.c_str(),
		LO_ARGS_END);
}

void
OSCEngineSender::request_variable(const string& object_path, const string& key)
{
	send("/ingen/request_variable", "iss",
		next_id(),
		object_path.c_str(),
		key.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::request_plugins()
{
	send("/ingen/request_plugins", "i", next_id(), LO_ARGS_END);
}


void
OSCEngineSender::request_all_objects()
{
	send("/ingen/request_all_objects", "i", next_id(), LO_ARGS_END);
}



} // namespace Client
} // namespace Ingen


