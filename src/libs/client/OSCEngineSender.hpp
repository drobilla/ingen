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

#ifndef OSCENGINESENDER_H
#define OSCENGINESENDER_H

#include <inttypes.h>
#include <string>
#include <lo/lo.h>
#include "interface/EngineInterface.hpp"
#include "shared/OSCSender.hpp"
using std::string;
using Ingen::Shared::EngineInterface;
using Ingen::Shared::ClientInterface;

namespace Ingen {
namespace Client {


/* OSC (via liblo) interface to the engine.
 *
 * Clients can use this opaquely as an EngineInterface* to control the engine
 * over OSC (whether over a network or not, etc).
 *
 * \ingroup IngenClient
 */
class OSCEngineSender : public EngineInterface, public Shared::OSCSender {
public:
	OSCEngineSender(const string& engine_url);

	~OSCEngineSender();
	
	string engine_url() { return _engine_url; }
	
	inline int32_t next_id()
	{ int32_t ret = (_id == -1) ? -1 : _id++; return ret; }

	void set_next_response_id(int32_t id) { _id = id; }
	void disable_responses() { _id = -1; }

	void attach(int32_t ping_id, bool block);


	/* *** EngineInterface implementation below here *** */
	
	void enable()  { _enabled = true; }
	void disable() { _enabled = false; }
	
	void bundle_begin()   { OSCSender::bundle_begin(); }
	void bundle_end()     { OSCSender::bundle_end(); }
	void transfer_begin() { OSCSender::transfer_begin(); }
	void transfer_end()   { OSCSender::transfer_end(); }

	// Client registration
	void register_client(ClientInterface* client);
	void unregister_client(const string& uri);

	// Engine commands
	void load_plugins();
	void activate();
	void deactivate();
	void quit();
	
	// Object commands
	
	void new_patch(const string& path,
	               uint32_t      poly);

	void new_port(const string& path,
	              uint32_t      index,
	              const string& data_type,
	              bool          is_output);

	void new_node(const string& path,
	              const string& plugin_uri,
	              bool          polyphonic);
	
	void new_node_deprecated(const string& path,
	                         const string& plugin_type,
	                         const string& library_name,
	                         const string& plugin_label,
	                         bool          polyphonic);

	void rename(const string& old_path,
	            const string& new_name);

	void destroy(const string& path);

	void clear_patch(const string& patch_path);
	
	void set_polyphony(const string& patch_path, uint32_t poly);
	
	void set_polyphonic(const string& path, bool poly);

	void connect(const string& src_port_path,
	             const string& dst_port_path);

	void disconnect(const string& src_port_path,
	                const string& dst_port_path);

	void disconnect_all(const string& parent_patch_path,
	                    const string& node_path);

	void set_port_value(const string&     port_path,
	                    const Raul::Atom& value);

	void set_voice_value(const string&     port_path,
	                     uint32_t          voice,
	                     const Raul::Atom& value);
	
	void set_port_value_immediate(const string&     port_path,
	        const Raul::Atom& value);
	
	void set_voice_value_immediate(const string&     port_path,
	                               uint32_t          voice,
	                               const Raul::Atom& value);
	
	void enable_port_broadcasting(const string& port_path);
	
	void disable_port_broadcasting(const string& port_path);

	void set_program(const string& node_path,
	                 uint32_t      bank,
	                 uint32_t      program);

	void midi_learn(const string& node_path);

	void set_variable(const string&     obj_path,
	                  const string&     predicate,
	                  const Raul::Atom& value);
	
	void set_property(const string&     obj_path,
	                  const string&     predicate,
	                  const Raul::Atom& value);
	
	// Requests //
	
	void ping();

	void request_plugin(const string& uri);

	void request_object(const string& path);

	void request_port_value(const string& port_path);
	
	void request_variable(const string& path, const string& key);

	void request_plugins();

	void request_all_objects();

protected:
	string     _engine_url;
	int        _client_port;
	int32_t    _id;
};


} // namespace Client
} // namespace Ingen

#endif // OSCENGINESENDER_H

