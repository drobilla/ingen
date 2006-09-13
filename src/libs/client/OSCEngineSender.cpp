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

#include <iostream>
#include "OSCEngineSender.h"
#include "interface/ClientKey.h"
#include "util/LibloAtom.h"
using std::cout; using std::cerr; using std::endl;

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
		string local_url = m_osc_listener->listen_url().substr(
			0, m_osc_listener->listen_url().find_last_of(":"));
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
			if (m_response_semaphore.try_wait() != 0) {
				cout << ".";
				cout.flush();
				ping(request_id);
				usleep(100000);
			} else {
				cout << " connected." << endl;
				m_waiting_for_response = false;
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
OSCEngineSender::register_client(ClientKey key, CountedPtr<ClientInterface> client)
{
	// FIXME: use parameters.. er, somehow.
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/engine/register_client", "i", next_id());
}


void
OSCEngineSender::unregister_client(ClientKey key)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/engine/unregister_client", "i", next_id());
}



// Engine commands
void
OSCEngineSender::load_plugins()
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/engine/load_plugins", "i", next_id());
}


void
OSCEngineSender::activate()    
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/engine/activate", "i", next_id());
}


void
OSCEngineSender::deactivate()  
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/engine/deactivate", "i", next_id());
}


void
OSCEngineSender::quit()        
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/engine/quit", "i", next_id());
}


		
// Object commands

void
OSCEngineSender::create_patch(const string& path,
                              uint32_t      poly)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/create_patch", "isi",
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
	lo_send(_engine_addr, "/om/synth/create_port",  "issi",
		next_id(),
		path.c_str(),
		data_type.c_str(),
		(is_output ? 1 : 0));
}


void
OSCEngineSender::create_node(const string& path,
                             const string& plugin_type,
                             const string& plugin_uri,
                             bool          polyphonic)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/create_node",  "isssi",
		next_id(),
		path.c_str(),
		plugin_type.c_str(),
		plugin_uri.c_str(),
		(polyphonic ? 1 : 0));
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
	lo_send(_engine_addr, "/om/synth/create_node",  "issssi",
		next_id(),
		path.c_str(),
		plugin_type.c_str(),
		library_name.c_str(),
		plugin_label.c_str(),
		(polyphonic ? 1 : 0));
}


void
OSCEngineSender::rename(const string& old_path,
                        const string& new_name)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/rename", "iss",
		next_id(),
		old_path.c_str(),
		new_name.c_str());
}


void
OSCEngineSender::destroy(const string& path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/destroy", "is",
		next_id(),
		path.c_str());
}


void
OSCEngineSender::clear_patch(const string& patch_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/clear_patch", "is",
		next_id(),
		patch_path.c_str());
}


void
OSCEngineSender::enable_patch(const string& patch_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/enable_patch", "is",
		next_id(),
		patch_path.c_str());
}


void
OSCEngineSender::disable_patch(const string& patch_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/disable_patch", "is",
		next_id(),
		patch_path.c_str());
}


void
OSCEngineSender::connect(const string& src_port_path,
                         const string& dst_port_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/connect", "iss",
		next_id(),
		src_port_path.c_str(),
		dst_port_path.c_str());
}


void
OSCEngineSender::disconnect(const string& src_port_path,
                            const string& dst_port_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/disconnect", "iss",
		next_id(),
		src_port_path.c_str(),
		dst_port_path.c_str());
}


void
OSCEngineSender::disconnect_all(const string& node_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/disconnect_all", "is",
		next_id(),
		node_path.c_str());
}


void
OSCEngineSender::set_port_value(const string& port_path,
                                float         val)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/set_port_value", "isf",
		next_id(),
		port_path.c_str(),
		val);
}


void
OSCEngineSender::set_port_value(const string& port_path,
                                uint32_t      voice,
                                float         val)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/set_port_value", "isif",
		next_id(),
		port_path.c_str(),
		voice,
		val);
}


void
OSCEngineSender::set_port_value_queued(const string& port_path,
                                       float         val)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/set_port_value_queued", "isf",
		next_id(),
		port_path.c_str(),
		val);
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
	lo_send(_engine_addr, "/om/synth/midi_learn", "is",
		next_id(),
		node_path.c_str());
}


void
OSCEngineSender::set_metadata(const string& obj_path,
                              const string& predicate,
                              const Atom&   value)
{
	
	assert(_engine_addr);
	lo_message m = lo_message_new();
	lo_message_add_int32(m, next_id());
	lo_message_add_string(m, obj_path.c_str());
	lo_message_add_string(m, predicate.c_str());
	LibloAtom::lo_message_add_atom(m, value);
	lo_send_message(_engine_addr, "/om/metadata/set", m);
}


// Requests //

void
OSCEngineSender::ping()
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/ping", "i", next_id());
}


void
OSCEngineSender::request_port_value(const string& port_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/request/port_value", "is",
		next_id(),
		port_path.c_str());
}


void
OSCEngineSender::request_plugins()
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/request/plugins", "i", next_id());
}


void
OSCEngineSender::request_all_objects()
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/request/all_objects", "i", next_id());
}



} // namespace Client
} // namespace Ingen


