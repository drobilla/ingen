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

#ifndef CLIENTINTERFACE_H
#define CLIENTINTERFACE_H

#include <inttypes.h>
#include <string>
using std::string;

namespace Ingen {
namespace Shared {


/** The (only) interface the engine uses to communicate with clients.
 *
 * Purely virtual (except for the destructor).
 */
class ClientInterface
{
public:
	
	virtual ~ClientInterface() {}
	
	virtual void response(int32_t id, bool success, string msg) = 0;

	/** Bundles are a group of messages that are guaranteed to be in an
	 * atomic unit with guaranteed order (eg a packet).  For datagram
	 * protocols (like UDP) there is likely an upper limit on bundle size.
	 */
	virtual void bundle_begin() = 0;
	virtual void bundle_end()   = 0;

	/** Transfers are 'weak' bundles.  These are used to break a large group
	 * of similar/related messages into larger chunks (solely for communication
	 * efficiency).  A bunch of messages in a transfer will arrive as 1 or more
	 * bundles (so a transfer can exceep the maximum bundle (packet) size).
	 */
	virtual void transfer_begin() = 0;
	virtual void transfer_end()   = 0;

	virtual void error(string msg) = 0;
	
	virtual void num_plugins(uint32_t num_plugins) = 0;

	virtual void new_plugin(string type,
	                        string uri,
	                        string name) = 0;
	
	virtual void new_patch(string path, uint32_t poly) = 0;
	
	virtual void new_node(string   plugin_type,
	                      string   plugin_uri,
	                      string   node_path,
	                      bool     is_polyphonic,
	                      uint32_t num_ports) = 0;
	
	virtual void new_port(string path,
	                      string data_type,
	                      bool   is_output) = 0;
	
	virtual void patch_enabled(string path) = 0;
	
	virtual void patch_disabled(string path) = 0;
	
	virtual void patch_cleared(string path) = 0;
	
	virtual void object_renamed(string old_path,
	                            string new_path) = 0;
	
	virtual void object_destroyed(string path) = 0;
	
	virtual void connection(string src_port_path,
	                        string dst_port_path) = 0;
	
	virtual void disconnection(string src_port_path,
	                           string dst_port_path) = 0;
	
	virtual void metadata_update(string subject_path,
	                             string predicate,
	                             string value) = 0;
	
	virtual void control_change(string port_path,
	                            float  value) = 0;
	
	virtual void program_add(string   node_path,
	                         uint32_t bank,
	                         uint32_t program,
	                         string   program_name) = 0;
	
	virtual void program_remove(string          node_path,
	                            uint32_t bank,
	                            uint32_t program) = 0;
	
protected:
	ClientInterface() {}
};


} // namespace Shared
} // namespace Ingen

#endif
