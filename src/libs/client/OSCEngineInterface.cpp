/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
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

#include "OSCEngineInterface.h"
#include "interface/ClientKey.h"

namespace LibOmClient {

/** Note the sending port is implicitly set by liblo, lo_send by default sends
 * from the most recently created server, so create the OSC listener before
 * this to have it all happen on the same port.  Yeah, this is a big magic :/
 */
OSCEngineInterface::OSCEngineInterface(const string& engine_url)
: _engine_url(engine_url)
, _engine_addr(lo_address_new_from_url(engine_url.c_str()))
, _id(0)
{
}


OSCEngineInterface::~OSCEngineInterface()
{
	lo_address_free(_engine_addr);
}


/* *** EngineInterface implementation below here *** */


/** Register with the engine via OSC.
 *
 * Note that this does not actually use 'key', since the engine creates
 * it's own key for OSC clients (namely the incoming URL), for NAT
 * traversal.  It is a parameter to remain compatible with EngineInterface.
 */
void
OSCEngineInterface::register_client(ClientKey key, CountedPtr<ClientInterface> client)
{
	// FIXME: use parameters.. er, somehow.
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/engine/register_client", "i", next_id());
}


void
OSCEngineInterface::unregister_client(ClientKey key)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/engine/unregister_client", "i", next_id());
}



// Engine commands
void
OSCEngineInterface::load_plugins()
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/engine/load_plugins", "i", next_id());
}


void
OSCEngineInterface::activate()    
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/engine/activate", "i", next_id());
}


void
OSCEngineInterface::deactivate()  
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/engine/deactivate", "i", next_id());
}


void
OSCEngineInterface::quit()        
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/engine/quit", "i", next_id());
}


		
// Object commands

void
OSCEngineInterface::create_patch(const string& path,
                                 uint32_t      poly)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/create_patch", "isi",
		next_id(),
		path.c_str(),
		poly);
}


void
OSCEngineInterface::create_port(const string& path,
                                const string& data_type,
                                bool          direction)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/create_port",  "issi",
		next_id(),
		path.c_str(),
		data_type.c_str(),
		(direction ? 1 : 0));
}


void
OSCEngineInterface::create_node(const string& path,
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


void
OSCEngineInterface::rename(const string& old_path,
                           const string& new_name)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/rename", "iss",
		next_id(),
		old_path.c_str(),
		new_name.c_str());
}


void
OSCEngineInterface::destroy(const string& path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/destroy", "is",
		next_id(),
		path.c_str());
}


void
OSCEngineInterface::clear_patch(const string& patch_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/clear_patch", "is",
		next_id(),
		patch_path.c_str());
}


void
OSCEngineInterface::enable_patch(const string& patch_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/enable_patch", "is",
		next_id(),
		patch_path.c_str());
}


void
OSCEngineInterface::disable_patch(const string& patch_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/disable_patch", "is",
		next_id(),
		patch_path.c_str());
}


void
OSCEngineInterface::connect(const string& src_port_path,
                            const string& dst_port_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/connect", "iss",
		next_id(),
		src_port_path.c_str(),
		dst_port_path.c_str());
}


void
OSCEngineInterface::disconnect(const string& src_port_path,
                               const string& dst_port_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/disconnect", "iss",
		next_id(),
		src_port_path.c_str(),
		dst_port_path.c_str());
}


void
OSCEngineInterface::disconnect_all(const string& node_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/disconnect_all", "is",
		next_id(),
		node_path.c_str());
}


void
OSCEngineInterface::set_port_value(const string& port_path,
                                   float         val)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/set_port_value", "isf",
		next_id(),
		port_path.c_str(),
		val);
}


void
OSCEngineInterface::set_port_value(const string& port_path,
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
OSCEngineInterface::set_port_value_queued(const string& port_path,
                                          float         val)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/set_port_value_queued", "isf",
		next_id(),
		port_path.c_str(),
		val);
}


void
OSCEngineInterface::set_program(const string& node_path,
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
OSCEngineInterface::midi_learn(const string& node_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/midi_learn", "is",
		next_id(),
		node_path.c_str());
}


void
OSCEngineInterface::set_metadata(const string& obj_path,
                                 const string& predicate,
                                 const string& value)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/metadata/set", "isss",
		next_id(),
		obj_path.c_str(),
		predicate.c_str(),
		value.c_str());
}


// Requests //

void
OSCEngineInterface::ping()
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/ping", "i", next_id());
}


void
OSCEngineInterface::request_port_value(const string& port_path)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/request/port_value", "is",
		next_id(),
		port_path.c_str());
}


void
OSCEngineInterface::request_plugins()
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/request/plugins", "i", next_id());
}


void
OSCEngineInterface::request_all_objects()
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/request/all_objects", "i", next_id());
}



} // namespace LibOmClient


