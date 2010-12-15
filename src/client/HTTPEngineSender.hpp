/* This file is part of Ingen.
 * Copyright (C) 2008-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_CLIENT_HTTPENGINESENDER_HPP
#define INGEN_CLIENT_HTTPENGINESENDER_HPP

#include <inttypes.h>

#include <string>

#include "raul/Path.hpp"
#include "redlandmm/World.hpp"

#include "interface/EngineInterface.hpp"

typedef struct _SoupSession SoupSession;

namespace Ingen {

namespace Shared { class World; }

namespace Client {

class HTTPClientReceiver;

/* HTTP (via libsoup) interface to the engine.
 *
 * Clients can use this opaquely as an EngineInterface to control the engine
 * over HTTP (whether over a network or not).
 *
 * \ingroup IngenClient
 */
class HTTPEngineSender : public Shared::EngineInterface
{
public:
	HTTPEngineSender(Shared::World* world, const Raul::URI& engine_url);
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

	virtual void put(const Raul::URI&                    path,
	                 const Shared::Resource::Properties& properties);

	virtual void delta(const Raul::URI&                    path,
	                   const Shared::Resource::Properties& remove,
	                   const Shared::Resource::Properties& add);

	virtual void del(const Raul::Path& path);

	virtual void move(const Raul::Path& old_path,
	                  const Raul::Path& new_path);

	virtual void connect(const Raul::Path& src_port_path,
	                     const Raul::Path& dst_port_path);

	virtual void disconnect(const Raul::Path& src_port_path,
	                        const Raul::Path& dst_port_path);

	virtual void disconnect_all(const Raul::Path& parent_patch_path,
	                            const Raul::Path& path);

	virtual void set_property(const Raul::URI&  subject,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value);

	// Requests //
	void ping();
	void get(const Raul::URI& uri);
	void request_property(const Raul::URI& path, const Raul::URI& key);

protected:
	SoupSession*    _session;
	Redland::World& _world;
	const Raul::URI _engine_url;
	int             _client_port;
	int32_t         _id;
	bool            _enabled;
};


} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_HTTPENGINESENDER_HPP

