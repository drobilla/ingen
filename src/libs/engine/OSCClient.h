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

#ifndef OSCCLIENT_H
#define OSCCLIENT_H

#include <string>
#include <iostream>
#include <list>
#include <lo/lo.h>
#include <pthread.h>
#include "types.h"
#include "interface/ClientInterface.h"

using std::list; using std::string;
using std::cerr;

namespace Om {


/** Implements ClientInterface for OSC clients (sends OSC messages).
 *
 * \ingroup engine
 */
class OSCClient : public Shared::ClientInterface
{
public:
	OSCClient(const string& url)
	: _url(url),
	  _address(lo_address_new_from_url(url.c_str()))
	{}

	virtual ~OSCClient()
	{ lo_address_free(_address); }

	const string&     url()     const  { return _url; }
	const lo_address  address() const  { return _address; }
	
	//void plugins(); // FIXME remove

	

	/* *** ClientInterface Implementation Below *** */


	//void client_registration(const string& url, int client_id);
	
	// need a liblo feature to make this possible :/
	void bundle_begin() {}
	void bundle_end()   {}

	void num_plugins(uint32_t num);

	void error(const string& msg);

	virtual void new_plugin(const string& type,
	                        const string& uri,
	                        const string& name);
	
	virtual void new_patch(const string& path, uint32_t poly);
	
	virtual void new_node(const string& plugin_type,
	                      const string& plugin_uri,
	                      const string& node_path,
	                      bool          is_polyphonic,
	                      uint32_t      num_ports);
	
	virtual void new_port(const string& path,
	                      const string& data_type,
	                      bool          is_output);
	
	virtual void patch_enabled(const string& path);
	
	virtual void patch_disabled(const string& path);
	
	virtual void patch_cleared(const string& path);
	
	virtual void object_destroyed(const string& path);
	
	virtual void object_renamed(const string& old_path,
	                            const string& new_path);
	
	virtual void connection(const string& src_port_path,
	                        const string& dst_port_path);
	
	virtual void disconnection(const string& src_port_path,
	                           const string& dst_port_path);
	
	virtual void metadata_update(const string& subject_path,
	                             const string& predicate,
	                             const string& value);
	
	virtual void control_change(const string& port_path,
	                            float         value);
	
	virtual void program_add(const string& node_path,
	                         uint32_t      bank,
	                         uint32_t      program,
	                         const string& program_name);
	
	virtual void program_remove(const string& node_path,
	                            uint32_t      bank,
	                            uint32_t      program);

private:
	// Prevent copies (undefined)
	OSCClient(const OSCClient&);
	OSCClient& operator=(const OSCClient&);
	
	string      _url;
	lo_address  _address;
};


} // namespace Om

#endif // OSCCLIENT_H

