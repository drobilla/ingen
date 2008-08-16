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

#ifndef OSCCLIENTSENDER_H
#define OSCCLIENTSENDER_H

#include <cassert>
#include <string>
#include <iostream>
#include <list>
#include <lo/lo.h>
#include <pthread.h>
#include "types.hpp"
#include "interface/ClientInterface.hpp"

using std::list;
using std::string;
using std::cerr;

namespace Ingen {

namespace Shared { class EngineInterface; }


/** Implements ClientInterface for OSC clients (sends OSC messages).
 *
 * \ingroup engine
 */
class OSCClientSender : public Shared::ClientInterface
{
public:
	OSCClientSender(const std::string& url)
		: Shared::ClientInterface(url)
		, _address(lo_address_new_from_url(url.c_str()))
		, _transfer(NULL)
		, _enabled(true)
	{}

	virtual ~OSCClientSender()
	{ lo_address_free(_address); }

	lo_address address() const { return _address; }
	
    void subscribe(Shared::EngineInterface* engine) { }

	/* *** ClientInterface Implementation Below *** */

	//void client_registration(const std::string& url, int client_id);
	
	void enable()  { _enabled = true; }
	void disable() { _enabled = false; }

	void bundle_begin();
	void bundle_end();
	
	void transfer_begin();
	void transfer_end();

	void response_ok(int32_t id);
	void response_error(int32_t id, const std::string& msg);

	void num_plugins(uint32_t num);

	void error(const std::string& msg);

	virtual void new_plugin(const std::string& uri,
	                        const std::string& type_uri,
	                        const std::string& symbol,
	                        const std::string& name);
	
	virtual void new_patch(const std::string& path, uint32_t poly);
	
	virtual void new_node(const std::string&   plugin_uri,
	                      const std::string&   node_path,
	                      bool                 is_polyphonic,
	                      uint32_t             num_ports);
	
	virtual void new_port(const std::string& path,
	                      uint32_t           index,
	                      const std::string& data_type,
	                      bool               is_output);
	
	virtual void polyphonic(const std::string& path,
	                        bool               polyphonic);
	
	virtual void patch_enabled(const std::string& path);
	
	virtual void patch_disabled(const std::string& path);
	
	virtual void patch_polyphony(const std::string& path,
	                             uint32_t           poly);
	
	virtual void patch_cleared(const std::string& path);
	
	virtual void object_destroyed(const std::string& path);
	
	virtual void object_renamed(const std::string& old_path,
	                            const std::string& new_path);
	
	virtual void connect(const std::string& src_port_path,
	                     const std::string& dst_port_path);
	
	virtual void disconnect(const std::string& src_port_path,
	                        const std::string& dst_port_path);
	
	virtual void set_variable(const std::string& subject_path,
	                          const std::string& predicate,
	                          const Raul::Atom&  value);
	
	virtual void control_change(const std::string& port_path,
	                            float              value);
	
	virtual void port_activity(const std::string& port_path);
	
	virtual void program_add(const std::string& node_path,
	                         uint32_t           bank,
	                         uint32_t           program,
	                         const std::string& program_name);
	
	virtual void program_remove(const std::string& node_path,
	                            uint32_t           bank,
	                            uint32_t           program);

private:
	int  send(const char *path, const char *types, ...);
	void send_message(const char* path, lo_message m);

	enum SendState { Immediate, SendingBundle, SendingTransfer };

	string     _url;
	lo_address _address;
	SendState  _send_state;
	lo_bundle  _transfer;
	bool       _enabled;
};


} // namespace Ingen

#endif // OSCCLIENTSENDER_H

