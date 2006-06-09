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

#ifndef OSCENGINEINTERFACE_H
#define OSCENGINEINTERFACE_H

#include <inttypes.h>
#include <string>
#include <lo/lo.h>
#include "interface/EngineInterface.h"
using std::string;
using Om::Shared::EngineInterface;
using Om::Shared::ClientInterface;
using Om::Shared::ClientKey;


namespace LibOmClient {

/* OSC (via liblo) interface to the engine.
 *
 * Clients can use this opaquely as an EngineInterface* to control the engine
 * over OSC (whether over a network or not, etc).
 *
 * \ingroup libomclient
 */
class OSCEngineInterface : public EngineInterface
{
public:
	OSCEngineInterface(const string& engine_url);

	~OSCEngineInterface();
	
	string engine_url() { return _engine_url; }
	
	inline size_t next_id()
	{ if (_id != -1) { _id = (_id == -2) ? 0 : _id+1; } return _id; }

	void enable_responses()  { _id = 0; }
	void disable_responses() { _id = -1; }


	/* *** EngineInterface implementation below here *** */

	// Client registration
	void register_client(ClientKey key, CountedPtr<ClientInterface> client);
	void unregister_client(ClientKey key);

	
	// Engine commands
	void load_plugins();
	void activate();
	void deactivate();
	void quit();

	
	// Object commands
	
	void create_patch(const string& path,
	                  uint32_t      poly);

	void create_node(const string& path,
	                 const string& plugin_type,
	                 const string& plugin_uri,
	                 bool          polyphonic);

	void rename(const string& old_path,
	            const string& new_name);

	void destroy(const string& path);

	void clear_patch(const string& patch_path);

	void enable_patch(const string& patch_path);

	void disable_patch(const string& patch_path);

	void connect(const string& src_port_path,
	             const string& dst_port_path);

	void disconnect(const string& src_port_path,
	                const string& dst_port_path);

	void disconnect_all(const string& node_path);

	void set_port_value(const string& port_path,
	                    float         val);

	void set_port_value(const string& port_path,
	                    uint32_t      voice,
	                    float         val);

	void set_port_value_queued(const string& port_path,
	                           float         val);

	void set_program(const string& node_path,
	                 uint32_t      bank,
	                 uint32_t      program);

	void midi_learn(const string& node_path);

	void set_metadata(const string& obj_path,
	                  const string& predicate,
	                  const string& value);
	
	// Requests //
	
	void ping();

	void request_port_value(const string& port_path);

	void request_plugins();

	void request_all_objects();

protected:
	string     _engine_url;
	lo_address _engine_addr;
	int        _client_port;
	int32_t    _id;
};


} // namespace LibOmClient

#endif // OSCENGINEINTERFACE_H

