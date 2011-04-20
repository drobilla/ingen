/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_CLIENT_OSCENGINESENDER_HPP
#define INGEN_CLIENT_OSCENGINESENDER_HPP

#include <stdbool.h>
#include <stddef.h>

#include <string>

#include <lo/lo.h>

#include "ingen/ServerInterface.hpp"
#include "shared/OSCSender.hpp"

namespace Ingen {

namespace Client {

/* OSC (via liblo) interface to the engine.
 *
 * Clients can use this opaquely as an ServerInterface* to control the engine
 * over OSC (whether over a network or not, etc).
 *
 * \ingroup IngenClient
 */
class OSCEngineSender : public ServerInterface, public Shared::OSCSender {
public:
	OSCEngineSender(const Raul::URI& engine_url,
	                size_t           max_packet_size);

	~OSCEngineSender();

	static OSCEngineSender* create(const Raul::URI& engine_url,
	                               size_t           max_packet_size) {
		return new OSCEngineSender(engine_url, max_packet_size);
	}

	Raul::URI uri() const { return _engine_url; }

	inline int32_t next_id()
	{ int32_t ret = (_id == -1) ? -1 : _id++; return ret; }

	void set_next_response_id(int32_t id) { _id = id; }
	void disable_responses() { _id = -1; }

	void attach(int32_t ping_id, bool block);

	/* *** ServerInterface implementation below here *** */

	void enable()  { _enabled = true; }
	void disable() { _enabled = false; }

	void bundle_begin()   { OSCSender::bundle_begin(); }
	void bundle_end()     { OSCSender::bundle_end(); }

	// Client registration
	void register_client(ClientInterface* client);
	void unregister_client(const Raul::URI& uri);

	// Object commands

	virtual void put(const Raul::URI&            path,
	                 const Resource::Properties& properties,
	                 Resource::Graph             ctx=Resource::DEFAULT);

	virtual void delta(const Raul::URI&            path,
	                   const Resource::Properties& remove,
	                   const Resource::Properties& add);

	virtual void del(const Raul::URI& uri);

	virtual void move(const Raul::Path& old_path,
	                  const Raul::Path& new_path);

	virtual void connect(const Raul::Path& src_port_path,
	                     const Raul::Path& dst_port_path);

	virtual void disconnect(const Raul::URI& src,
	                        const Raul::URI& dst);

	virtual void disconnect_all(const Raul::Path& parent_patch_path,
	                            const Raul::Path& path);

	virtual void set_property(const Raul::URI&  subject_path,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value);

	// Requests //
	void ping();
	void get(const Raul::URI& uri);
	void request_property(const Raul::URI& path, const Raul::URI& key);

protected:
	const Raul::URI _engine_url;
	int             _client_port;
	int32_t         _id;
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_OSCENGINESENDER_HPP

