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
#include <lo/lo.h>
#include <pthread.h>
#include "types.hpp"
#include "interface/ClientInterface.hpp"
#include "shared/OSCSender.hpp"

namespace Ingen {

namespace Shared { class EngineInterface; }


/** Implements ClientInterface for OSC clients (sends OSC messages).
 *
 * \ingroup engine
 */
class OSCClientSender : public Shared::ClientInterface, public Shared::OSCSender
{
public:
	OSCClientSender(const std::string& url)
		: _url(url)
	{
		_address = lo_address_new_from_url(url.c_str());
	}

	virtual ~OSCClientSender()
	{ lo_address_free(_address); }
	
	void enable()  { _enabled = true; }
	void disable() { _enabled = false; }
	
	void bundle_begin()   { OSCSender::bundle_begin(); }
	void bundle_end()     { OSCSender::bundle_end(); }
	void transfer_begin() { OSCSender::transfer_begin(); }
	void transfer_end()   { OSCSender::transfer_end(); }
	
	std::string uri() const { return lo_address_get_url(_address); }
	
    void subscribe(Shared::EngineInterface* engine) { }

	/* *** ClientInterface Implementation Below *** */

	//void client_registration(const std::string& url, int client_id);

	void response_ok(int32_t id);
	void response_error(int32_t id, const std::string& msg);

	void error(const std::string& msg);

	virtual void new_plugin(const std::string& uri,
	                        const std::string& type_uri,
	                        const std::string& symbol,
	                        const std::string& name);
	
	virtual void new_patch(const std::string& path, uint32_t poly);
	
	virtual void new_node(const std::string&   path,
	                      const std::string&   plugin_uri);
	
	virtual void new_port(const std::string& path,
	                      uint32_t           index,
	                      const std::string& data_type,
	                      bool               is_output);
	
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
	
	virtual void set_property(const std::string& subject_path,
	                          const std::string& predicate,
	                          const Raul::Atom&  value);
	
	virtual void set_port_value(const std::string& port_path,
	                            const Raul::Atom&  value);
	
	virtual void set_voice_value(const std::string& port_path,
	                             uint32_t           voice,
	                             const Raul::Atom&  value);
	
	virtual void port_activity(const std::string& port_path);
	
	virtual void program_add(const std::string& node_path,
	                         uint32_t           bank,
	                         uint32_t           program,
	                         const std::string& program_name);
	
	virtual void program_remove(const std::string& node_path,
	                            uint32_t           bank,
	                            uint32_t           program);

private:
	std::string _url;
};


} // namespace Ingen

#endif // OSCCLIENTSENDER_H

