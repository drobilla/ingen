/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

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

#include "ingen/Message.hpp"
#include "ingen/Resource.hpp"
#include "ingen/Status.hpp"
#include "ingen/ingen.h"
#include "ingen/types.hpp"

namespace Raul {
class Atom;
class Path;
class URI;
}

namespace Ingen {

/** Abstract interface for Ingen servers and clients.
 *
 * @ingroup Ingen
 */
class INGEN_API Interface
{
public:
	virtual ~Interface() {}

	virtual Raul::URI uri() const = 0;

	virtual SPtr<Interface> respondee() const {
		return SPtr<Interface>();
	}

	virtual void set_respondee(SPtr<Interface> respondee) {}

	/** Set the ID to use to respond to the next message.
	 * Setting the ID to 0 will disable responses.
	 */
	virtual void set_response_id(int32_t id) = 0;

	virtual void message(const Message& message) = 0;

	inline void bundle_begin() { message(BundleBegin{}); }

	inline void bundle_end() { message(BundleEnd{}); }

	inline void put(const Raul::URI&  uri,
	                const Properties& properties,
	                Resource::Graph   ctx = Resource::Graph::DEFAULT)
	{
		message(Put{uri, properties, ctx});
	}

	inline void delta(const Raul::URI&  uri,
	                  const Properties& remove,
	                  const Properties& add,
	                  Resource::Graph   ctx = Resource::Graph::DEFAULT)
	{
		message(Delta{uri, remove, add, ctx});
	}

	inline void copy(const Raul::URI& old_uri, const Raul::URI& new_uri)
	{
		message(Copy{old_uri, new_uri});
	}

	inline void move(const Raul::Path& old_path, const Raul::Path& new_path)
	{
		message(Move{old_path, new_path});
	}

	inline void del(const Raul::URI& uri) { message(Del{uri}); }

	inline void connect(const Raul::Path& tail, const Raul::Path& head)
	{
		message(Connect{tail, head});
	}

	inline void disconnect(const Raul::Path& tail, const Raul::Path& head)
	{
		message(Disconnect{tail, head});
	}

	inline void disconnect_all(const Raul::Path& graph, const Raul::Path& path)
	{
		message(DisconnectAll{graph, path});
	}

	inline void set_property(const Raul::URI& subject,
	                         const Raul::URI& predicate,
	                         const Atom&      value,
	                         Resource::Graph  ctx = Resource::Graph::DEFAULT)
	{
		message(SetProperty{subject, predicate, value, ctx});
	}

	inline void undo() { message(Undo{}); }

	inline void redo() { message(Redo{}); }

	inline void get(const Raul::URI& uri) { message(Get{uri}); }

	inline void response(int32_t            id,
	                     Status             status,
	                     const std::string& subject) {
		message(Response{id, status, subject});
	}

	inline void error(const std::string& error_message) {
		message(Error{error_message});
	}
};

} // namespace Ingen

#endif // INGEN_INTERFACE_HPP
