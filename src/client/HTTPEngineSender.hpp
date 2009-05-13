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

#ifndef HTTPENGINESENDER_H
#define HTTPENGINESENDER_H

#include <inttypes.h>
#include <string>
#include <libsoup/soup.h>
#include "raul/Path.hpp"
#include "interface/EngineInterface.hpp"

namespace Ingen {
namespace Client {


/* HTTP (via libsoup) interface to the engine.
 *
 * Clients can use this opaquely as an EngineInterface to control the engine
 * over HTTP (whether over a network or not).
 *
 * \ingroup IngenClient
 */
class HTTPEngineSender : public Shared::EngineInterface {
public:
	HTTPEngineSender(const Raul::URI& engine_url);
	~HTTPEngineSender();
	
	Raul::URI uri() const { return _engine_url; }
	
	inline int32_t next_id()
	{ int32_t ret = (_id == -1) ? -1 : _id++; return ret; }

	void set_next_response_id(int32_t id) { _id = id; }
	void disable_responses() { _id = -1; }

	void attach(int32_t ping_id, bool block);


	/* *** EngineInterface implementation below here *** */
	
	void enable()  { _enabled = true; }
	void disable() { _enabled = false; }
	
	void bundle_begin()   { transfer_begin(); }
	void bundle_end()     { transfer_end(); }
	
	void transfer_begin() {}
	void transfer_end()   {}

	// Client registration
	void register_client(Shared::ClientInterface* client);
	void unregister_client(const Raul::URI& uri);

	// Engine commands
	void load_plugins();
	void activate();
	void deactivate();
	void quit();
	
	// Object commands
	
	virtual bool new_object(const Shared::GraphObject* object);

	virtual void new_patch(const Raul::Path& path,
	                       uint32_t          poly);
	
	virtual void new_node(const Raul::Path& path,
	                      const Raul::URI&  plugin_uri);
	
	virtual void new_port(const Raul::Path& path,
	                      const Raul::URI&  type,
	                      uint32_t          index,
	                      bool              is_output);
	
	virtual void clear_patch(const Raul::Path& path);
	
	virtual void destroy(const Raul::Path& path);
	
	virtual void rename(const Raul::Path& old_path,
	                    const Raul::Path& new_path);
	
	virtual void connect(const Raul::Path& src_port_path,
	                     const Raul::Path& dst_port_path);
	
	virtual void disconnect(const Raul::Path& src_port_path,
	                        const Raul::Path& dst_port_path);
	
	virtual void disconnect_all(const Raul::Path& parent_patch_path,
	                            const Raul::Path& path);
	
	virtual void set_variable(const Raul::Path& subject_path,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value);
	
	virtual void set_property(const Raul::Path& subject_path,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value);
	
	virtual void set_port_value(const Raul::Path& port_path,
	                            const Raul::Atom& value);
	
	virtual void set_voice_value(const Raul::Path& port_path,
	                             uint32_t          voice,
	                             const Raul::Atom& value);
	
	virtual void set_program(const Raul::Path& node_path,
	                 uint32_t                  bank,
	                 uint32_t                  program);

	virtual void midi_learn(const Raul::Path& node_path);
	

	// Requests //
	void ping();
	void request_plugin(const Raul::URI& uri);
	void request_object(const Raul::Path& path);
	void request_port_value(const Raul::Path& port_path);
	void request_variable(const Raul::Path& path, const Raul::URI& key);
	void request_property(const Raul::Path& path, const Raul::URI& key);
	void request_plugins();
	void request_all_objects();

protected:
	SoupSession*    _session;
	const Raul::URI _engine_url;
	int             _client_port;
	int32_t         _id;
	bool            _enabled;
};


} // namespace Client
} // namespace Ingen

#endif // HTTPENGINESENDER_H

