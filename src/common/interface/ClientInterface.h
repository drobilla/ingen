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

#ifndef CLIENTINTERFACE_H
#define CLIENTINTERFACE_H

#include <inttypes.h>
#include <string>
using std::string;

namespace Om {
namespace Shared {


/** The (only) interface the engine uses to communicate with clients.
 *
 * Purely virtual (except for the destructor).
 */
class ClientInterface
{
public:
	
	virtual ~ClientInterface() {}
	
	virtual void bundle_begin() = 0;
	virtual void bundle_end()   = 0;

	virtual void error(const string& msg) = 0;
	
	virtual void num_plugins(uint32_t num_plugins) = 0;

	virtual void new_plugin(const string& type,
	                        const string& uri,
	                        const string& name) = 0;
	
	virtual void new_patch(const string& path, uint32_t poly) = 0;
	
	virtual void new_node(const string& plugin_type,
	                      const string& plugin_uri,
	                      const string& node_path,
	                      bool          is_polyphonic,
	                      uint32_t      num_ports) = 0;
	
	virtual void new_port(const string& path,
	                      const string& data_type,
	                      bool          is_output) = 0;
	
	virtual void patch_enabled(const string& path) = 0;
	
	virtual void patch_disabled(const string& path) = 0;
	
	virtual void patch_cleared(const string& path) = 0;
	
	virtual void object_renamed(const string& old_path,
	                            const string& new_path) = 0;
	
	virtual void object_destroyed(const string& path) = 0;
	
	virtual void connection(const string& src_port_path,
	                        const string& dst_port_path) = 0;
	
	virtual void disconnection(const string& src_port_path,
	                           const string& dst_port_path) = 0;
	
	virtual void metadata_update(const string& subject_path,
	                             const string& predicate,
	                             const string& value) = 0;
	
	virtual void control_change(const string& port_path,
	                            float         value) = 0;
	
	virtual void program_add(const string& node_path,
	                         uint32_t      bank,
	                         uint32_t      program,
	                         const string& program_name) = 0;
	
	virtual void program_remove(const string& node_path,
	                            uint32_t      bank,
	                            uint32_t      program) = 0;
	
protected:
	ClientInterface() {}
};


} // namespace Shared
} // namespace Om

#endif
