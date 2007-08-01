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

#ifndef CLIENTINTERFACE_H
#define CLIENTINTERFACE_H

#include <stdio.h>
#include <string>
#include <inttypes.h>
#include <raul/Atom.hpp>

namespace Ingen {
namespace Shared {

class EngineInterface;


/** The (only) interface the engine uses to communicate with clients.
 *
 * Purely virtual (except for the destructor).
 */
class ClientInterface
{
public:
	
	virtual ~ClientInterface() {}

	inline const std::string& uri() const { return _uri; }

	virtual void response_ok(int32_t id) = 0;

	virtual void response_error(int32_t id, const std::string& msg) = 0;
	
	virtual void enable() = 0;
	
	/** Signifies the client does not wish to receive any messages until
	 * enable is called.  Useful for performance and avoiding feedback.
	 */
	virtual void disable() = 0;

	/** Bundles are a group of messages that are guaranteed to be in an
	 * atomic unit with guaranteed order (eg a packet).  For datagram
	 * protocols (like UDP) there is likely an upper limit on bundle size.
	 */
	virtual void bundle_begin() = 0;
	virtual void bundle_end()   = 0;
	
	/** Transfers are 'weak' bundles.  These are used to break a large group
	 * of similar/related messages into larger chunks (solely for communication
	 * efficiency).  A bunch of messages in a transfer will arrive as 1 or more
	 * bundles (so a transfer can exceed the maximum bundle (packet) size).
	 */
	virtual void transfer_begin() = 0;
	virtual void transfer_end()   = 0;
	
	virtual void error(const std::string& msg) = 0;
	
	virtual void num_plugins(uint32_t num_plugins) = 0;
	
	virtual void new_plugin(const std::string& uri,
	                        const std::string& type_uri,
	                        const std::string& name) = 0;
	
	virtual void new_patch(const std::string& path, uint32_t poly) = 0;
	
	virtual void new_node(const std::string& plugin_uri,
	                      const std::string& node_path,
	                      bool               is_polyphonic,
	                      uint32_t           num_ports) = 0;
	
	virtual void new_port(const std::string& path,
	                      const std::string& data_type,
	                      bool               is_output) = 0;
	
	virtual void patch_enabled(const std::string& path) = 0;
	
	virtual void patch_disabled(const std::string& path) = 0;
	
	virtual void patch_cleared(const std::string& path) = 0;
	
	virtual void object_renamed(const std::string& old_path,
	                            const std::string& new_path) = 0;
	
	virtual void object_destroyed(const std::string& path) = 0;
	
	virtual void connection(const std::string& src_port_path,
	                        const std::string& dst_port_path) = 0;
	
	virtual void disconnection(const std::string& src_port_path,
	                           const std::string& dst_port_path) = 0;
	
	virtual void metadata_update(const std::string& subject_path,
	                             const std::string& predicate,
	                             const Raul::Atom&  value) = 0;
	
	virtual void control_change(const std::string& port_path,
	                            float              value) = 0;
	
	virtual void program_add(const std::string& node_path,
	                         uint32_t           bank,
	                         uint32_t           program,
	                         const std::string& program_name) = 0;
	
	virtual void program_remove(const std::string& node_path,
	                            uint32_t           bank,
	                            uint32_t           program) = 0;
	
protected:
	ClientInterface(const std::string& uri) : _uri(uri) {}
	ClientInterface() {
		static char uri_buf[20];
		snprintf(uri_buf, 127, "%p", this);
		_uri = uri_buf;
	}

	std::string _uri;
};


} // namespace Shared
} // namespace Ingen

#endif
