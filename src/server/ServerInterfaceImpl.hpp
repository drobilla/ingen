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

#ifndef INGEN_ENGINE_QUEUEDENGINEINTERFACE_HPP
#define INGEN_ENGINE_QUEUEDENGINEINTERFACE_HPP

#include <inttypes.h>
#include <string>
#include <memory>
#include "raul/SharedPtr.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Resource.hpp"
#include "EventSource.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {

class Engine;

/** A queued (preprocessed) event source / interface.
 *
 * This is the bridge between the Interface presented to the client, and
 * the EventSource that needs to be presented to the Driver.
 *
 * Responses occur through the event mechanism (which notified clients in
 * event post_process methods) and are related to an event by an integer ID.
 * If you do not register a request, you have no way of knowing if your calls
 * are successful.
 */
class ServerInterfaceImpl : public EventSource,
                            public Interface
{
public:
	explicit ServerInterfaceImpl(Engine& engine);
	virtual ~ServerInterfaceImpl();

	Raul::URI uri() const { return "http://drobilla.net/ns/ingen#internal"; }

	void set_response_interface(Interface* iface) { _request_client = iface; }

	virtual void set_response_id(int32_t id);

	virtual void bundle_begin();

	virtual void bundle_end();

	virtual void put(const Raul::URI&            path,
	                 const Resource::Properties& properties,
	                 const Resource::Graph       g=Resource::DEFAULT);

	virtual void delta(const Raul::URI&            path,
	                   const Resource::Properties& remove,
	                   const Resource::Properties& add);

	virtual void move(const Raul::Path& old_path,
	                  const Raul::Path& new_path);

	virtual void connect(const Raul::Path& tail,
	                     const Raul::Path& head);

	virtual void disconnect(const Raul::Path& tail,
	                        const Raul::Path& head);

	virtual void set_property(const Raul::URI& subject_path,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value);

	virtual void del(const Raul::URI& uri);

	virtual void disconnect_all(const Raul::Path& parent_patch_path,
	                            const Raul::Path& path);

	virtual void get(const Raul::URI& uri);

	virtual void response(int32_t id, Status status) {}  ///< N/A
	virtual void error(const std::string& msg) {}  ///< N/A

protected:
	Interface* _request_client;
	int32_t    _request_id;
	Engine&    _engine;
	bool       _in_bundle;  ///< True iff a bundle is currently being received

private:
	SampleCount now() const;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_QUEUEDENGINEINTERFACE_HPP
