/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef INGEN_CLIENT_HTTPENGINESENDER_HPP
#define INGEN_CLIENT_HTTPENGINESENDER_HPP

#include <inttypes.h>

#include <string>

#include "raul/Deletable.hpp"
#include "raul/Path.hpp"
#include "sord/sordmm.hpp"

#include "ingen/ServerInterface.hpp"

typedef struct _SoupSession SoupSession;

namespace Ingen {

namespace Shared { class World; }

namespace Client {

class HTTPClientReceiver;

/* HTTP (via libsoup) interface to the engine.
 *
 * Clients can use this opaquely as an ServerInterface to control the engine
 * over HTTP (whether over a network or not).
 *
 * \ingroup IngenClient
 */
class HTTPEngineSender : public ServerInterface
{
public:
	HTTPEngineSender(Shared::World*             world,
	                 const Raul::URI&           engine_url,
	                 SharedPtr<Raul::Deletable> receiver);

	~HTTPEngineSender();

	Raul::URI uri() const { return _engine_url; }

	inline int32_t next_id()
	{ int32_t ret = (_id == -1) ? -1 : _id++; return ret; }

	void set_response_id(int32_t id) { _id = id; }

	void attach(int32_t ping_id, bool block);

	/* *** ServerInterface implementation below here *** */

	void enable()  { _enabled = true; }
	void disable() { _enabled = false; }

	void bundle_begin() {}
	void bundle_end()   {}

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

	virtual void set_property(const Raul::URI&  subject,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value);

	// Requests //
	void ping();
	void get(const Raul::URI& uri);

protected:
	SharedPtr<HTTPClientReceiver> _receiver;

	SoupSession*    _session;
	Sord::World&    _world;
	const Raul::URI _engine_url;
	int             _client_port;
	int32_t         _id;
	bool            _enabled;
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_HTTPENGINESENDER_HPP

