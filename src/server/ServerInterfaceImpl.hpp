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

#ifndef INGEN_ENGINE_QUEUEDENGINEINTERFACE_HPP
#define INGEN_ENGINE_QUEUEDENGINEINTERFACE_HPP

#include <inttypes.h>
#include <string>
#include <memory>
#include "raul/SharedPtr.hpp"
#include "ingen/ClientInterface.hpp"
#include "ingen/ServerInterface.hpp"
#include "ingen/Resource.hpp"
#include "EventSource.hpp"
#include "Request.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {

class Engine;

/** A queued (preprocessed) event source / interface.
 *
 * This is the bridge between the ServerInterface presented to the client, and
 * the EventSource that needs to be presented to the Driver.
 *
 * Responses occur through the event mechanism (which notified clients in
 * event post_process methods) and are related to an event by an integer ID.
 * If you do not register a request, you have no way of knowing if your calls
 * are successful.
 */
class ServerInterfaceImpl : public EventSource,
                              public ServerInterface
{
public:
	ServerInterfaceImpl(Engine& engine);
	virtual ~ServerInterfaceImpl();

	Raul::URI uri() const { return "http://drobilla.net/ns/ingen#internal"; }

	void set_next_response_id(int32_t id);

	// Client registration
	virtual void register_client(ClientInterface* client);
	virtual void unregister_client(const Raul::URI& uri);

	// Bundles
	virtual void bundle_begin();
	virtual void bundle_end();

	// CommonInterface object commands

	virtual void put(const Raul::URI&            path,
	                 const Resource::Properties& properties,
	                 const Resource::Graph       g=Resource::DEFAULT);

	virtual void delta(const Raul::URI&            path,
	                   const Resource::Properties& remove,
	                   const Resource::Properties& add);

	virtual void move(const Raul::Path& old_path,
	                  const Raul::Path& new_path);

	virtual void connect(const Raul::Path& src_port_path,
	                     const Raul::Path& dst_port_path);

	virtual void disconnect(const Raul::URI& src,
	                        const Raul::URI& dst);

	virtual void set_property(const Raul::URI& subject_path,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value);

	virtual void del(const Raul::URI& uri);

	// ServerInterface object commands

	virtual void disconnect_all(const Raul::Path& parent_patch_path,
	                            const Raul::Path& path);

	// Requests
	virtual void ping();
	virtual void get(const Raul::URI& uri);

protected:
	virtual void disable_responses();

	SharedPtr<Request> _request; ///< NULL if responding disabled
	Engine&            _engine;
	bool               _in_bundle; ///< True iff a bundle is currently being received

private:
	SampleCount now() const;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_QUEUEDENGINEINTERFACE_HPP
