/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
 * 
 * Ingen is free software = 0; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation = 0; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY = 0; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program = 0; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef ENGINEINTERFACE_H
#define ENGINEINTERFACE_H

#include <inttypes.h>
#include <string>
#include "util/CountedPtr.h"
#include "interface/ClientInterface.h"
using std::string;

namespace Om {
/** Shared code used on both client side and engine side (abstract interfaces). */
namespace Shared {

class ClientKey;


/** The (only) interface clients use to communicate with the engine.
 *
 * Purely virtual (except for the destructor).
 */
class EngineInterface
{
public:
	virtual ~EngineInterface() {}
	
	// Client registration
	virtual void register_client(ClientKey key, CountedPtr<ClientInterface> client) = 0;
	virtual void unregister_client(ClientKey key) = 0;
	

	// Engine commands
	virtual void load_plugins() = 0;
	virtual void activate() = 0;
	virtual void deactivate() = 0;
	virtual void quit() = 0;
			
	// Object commands
	
	virtual void create_patch(const string& path,
	                          uint32_t      poly) = 0;

	virtual void create_port(const string& path,
	                         const string& data_type,
	                         bool          direction) = 0;
	
	virtual void create_node(const string& path,
	                         const string& plugin_type,
	                         const string& plugin_uri,
				        	 bool          polyphonic) = 0;

	virtual void rename(const string& old_path,
	                    const string& new_name) = 0;

	virtual void destroy(const string& path) = 0;

	virtual void clear_patch(const string& patch_path) = 0;

	virtual void enable_patch(const string& patch_path) = 0;

	virtual void disable_patch(const string& patch_path) = 0;

	virtual void connect(const string& src_port_path,
	                     const string& dst_port_path) = 0;

	virtual void disconnect(const string& src_port_path,
	                        const string& dst_port_path) = 0;

	virtual void disconnect_all(const string& node_path) = 0;

	virtual void set_port_value(const string& port_path,
	                            float         val) = 0;

	virtual void set_port_value(const string& port_path,
	                            uint32_t      voice,
	                            float         val) = 0;

	virtual void set_port_value_queued(const string& port_path,
	                                   float         val) = 0;

	virtual void set_program(const string& node_path,
	                         uint32_t      bank,
	                         uint32_t      program) = 0;

	virtual void midi_learn(const string& node_path) = 0;

	virtual void set_metadata(const string& path,
	                          const string& predicate,
	                          const string& value) = 0;
	
	// Requests //
	
	virtual void ping() = 0;

	virtual void request_port_value(const string& port_path) = 0;

	virtual void request_plugins() = 0;

	virtual void request_all_objects() = 0;

protected:
	EngineInterface() {}
};


} // namespace Shared
} // namespace Om

#endif // ENGINEINTERFACE_H

