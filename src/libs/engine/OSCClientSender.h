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

#ifndef OSCCLIENTSENDER_H
#define OSCCLIENTSENDER_H

#include <cassert>
#include <string>
#include <iostream>
#include <list>
#include <lo/lo.h>
#include <pthread.h>
#include "types.h"
#include "interface/ClientInterface.h"

using std::list; using std::string;
using std::cerr;

namespace Ingen {


/** Implements ClientInterface for OSC clients (sends OSC messages).
 *
 * \ingroup engine
 */
class OSCClientSender : public Shared::ClientInterface
{
public:
	OSCClientSender(const string& url)
	: _url(url),
	  _address(lo_address_new_from_url(url.c_str())),
	  _transfer(NULL),
	  _enabled(true)
	{}

	virtual ~OSCClientSender()
	{ lo_address_free(_address); }

	const string&     url()     const  { return _url; }
	const lo_address  address() const  { return _address; }
	
	//void plugins(); // FIXME remove

	

	/* *** ClientInterface Implementation Below *** */


	//void client_registration(string url, int client_id);
	
	void enable()  { _enabled = true; }
	void disable() { _enabled = false; }

	void bundle_begin();
	void bundle_end();
	
	void transfer_begin();
	void transfer_end();

	void response(int32_t id, bool success, string msg);

	void num_plugins(uint32_t num);

	void error(string msg);

	virtual void new_plugin(string uri,
	                        string name);
	
	virtual void new_patch(string path, uint32_t poly);
	
	virtual void new_node(string   plugin_uri,
	                      string   node_path,
	                      bool     is_polyphonic,
	                      uint32_t num_ports);
	
	virtual void new_port(string path,
	                      string data_type,
	                      bool   is_output);
	
	virtual void patch_enabled(string path);
	
	virtual void patch_disabled(string path);
	
	virtual void patch_cleared(string path);
	
	virtual void object_destroyed(string path);
	
	virtual void object_renamed(string old_path,
	                            string new_path);
	
	virtual void connection(string src_port_path,
	                        string dst_port_path);
	
	virtual void disconnection(string src_port_path,
	                           string dst_port_path);
	
	virtual void metadata_update(string subject_path,
	                             string predicate,
	                             Atom   value);
	
	virtual void control_change(string port_path,
	                            float  value);
	
	virtual void program_add(string   node_path,
	                         uint32_t bank,
	                         uint32_t program,
	                         string   program_name);
	
	virtual void program_remove(string   node_path,
	                            uint32_t bank,
	                            uint32_t program);

private:
	string      _url;
	lo_address  _address;

	lo_bundle _transfer;

	bool _enabled;
};


} // namespace Ingen

#endif // OSCCLIENTSENDER_H

