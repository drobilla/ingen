/* This file is part of Ingen.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
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

#ifndef HTTPCLIENTSENDER_H
#define HTTPCLIENTSENDER_H

#include <cassert>
#include <string>
#include <iostream>
#include <lo/lo.h>
#include <pthread.h>
#include "types.hpp"
#include "raul/Thread.hpp"
#include "interface/ClientInterface.hpp"
#include "shared/HTTPSender.hpp"

namespace Ingen {

class Engine;

namespace Shared { class EngineInterface; }


/** Implements ClientInterface for HTTP clients.
 * Sends changes as RDF deltas over an HTTP stream
 * (a single message with chunked encoding response).
 *
 * \ingroup engine
 */
class HTTPClientSender
	: public Shared::ClientInterface
	, public Shared::HTTPSender
{
public:
	HTTPClientSender(Engine& engine)
		: _engine(engine)
	{}

	bool enabled() const { return _enabled; }

	void enable()  { _enabled = true; }
	void disable() { _enabled = false; }
	
	void bundle_begin()   { HTTPSender::bundle_begin(); }
	void bundle_end()     { HTTPSender::bundle_end(); }
	void transfer_begin() { HTTPSender::transfer_begin(); }
	void transfer_end()   { HTTPSender::transfer_end(); }

	std::string uri() const { return "http://example.org/"; }
	
    void subscribe(Shared::EngineInterface* engine) { }

	/* *** ClientInterface Implementation Below *** */

	//void client_registration(const std::string& url, int client_id);

	void response_ok(int32_t id);
	void response_error(int32_t id, const std::string& msg);

	void error(const std::string& msg);
	
	virtual bool new_object(const Shared::GraphObject* object);

	virtual void new_plugin(const std::string& uri,
	                        const std::string& type_uri,
	                        const std::string& symbol,
	                        const std::string& name);
	
	virtual void new_patch(const std::string& path, uint32_t poly);
	
	virtual void new_node(const std::string&   path,
	                      const std::string&   plugin_uri);
	
	virtual void new_port(const std::string& path,
	                      const std::string& type,
	                      uint32_t           index,
	                      bool               is_output);
	
	virtual void patch_cleared(const std::string& path);
	
	virtual void destroy(const std::string& path);
	
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
	
	virtual void activity(const std::string& path);
	
	virtual void program_add(const std::string& node_path,
	                         uint32_t           bank,
	                         uint32_t           program,
	                         const std::string& program_name);
	
	virtual void program_remove(const std::string& node_path,
	                            uint32_t           bank,
	                            uint32_t           program);

private:
	Engine&     _engine;
	std::string _url;
	bool        _enabled;
};


} // namespace Ingen

#endif // HTTPCLIENTSENDER_H

