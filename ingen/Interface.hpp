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

/**
   @defgroup Ingen Core Interfaces
*/

#ifndef INGEN_INTERFACE_HPP
#define INGEN_INTERFACE_HPP

#include <string>

#include "ingen/Resource.hpp"
#include "ingen/Status.hpp"
#include "ingen/types.hpp"

namespace Raul {
class Atom;
class Path;
class URI;
}

namespace Ingen {

/** Abstract interface common to both engine and clients.
 * Purely virtual (except for the destructor).
 *
 * @ingroup Ingen
 */
class Interface
{
public:
	virtual ~Interface() {}

	virtual Raul::URI uri() const = 0;

	virtual SPtr<Interface> respondee() const {
		return SPtr<Interface>();
	}

	virtual void set_respondee(SPtr<Interface> respondee) {}

	/** Begin an atomic bundle */
	virtual void bundle_begin() = 0;

	/** End (and send) an atomic bundle */
	virtual void bundle_end() = 0;

	virtual void put(const Raul::URI&            uri,
	                 const Resource::Properties& properties,
	                 Resource::Graph             ctx=Resource::Graph::DEFAULT) = 0;

	virtual void delta(const Raul::URI&            uri,
	                   const Resource::Properties& remove,
	                   const Resource::Properties& add) = 0;

	virtual void move(const Raul::Path& old_path,
	                  const Raul::Path& new_path) = 0;

	virtual void del(const Raul::URI& uri) = 0;

	virtual void connect(const Raul::Path& tail,
	                     const Raul::Path& head) = 0;

	virtual void disconnect(const Raul::Path& tail,
	                        const Raul::Path& head) = 0;

	virtual void disconnect_all(const Raul::Path& graph,
	                            const Raul::Path& path) = 0;

	virtual void set_property(const Raul::URI& subject,
	                          const Raul::URI& predicate,
	                          const Atom&      value) = 0;

	/** Set the ID to use to respond to the next message.
	 * Setting the ID to -1 will disable responses.
	 */
	virtual void set_response_id(int32_t id) = 0;

	// Requests
	virtual void get(const Raul::URI& uri) = 0;

	// Response
	virtual void response(int32_t            id,
	                      Status             status,
	                      const std::string& subject) = 0;

	// Non-response error
	virtual void error(const std::string& msg) = 0;
};

} // namespace Ingen

#endif // INGEN_INTERFACE_HPP
