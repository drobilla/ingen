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

#include <cstdint>
#include <string>

#include "ingen/Message.hpp"
#include "ingen/Properties.hpp"
#include "ingen/Resource.hpp"
#include "ingen/Status.hpp"
#include "ingen/ingen.h"
#include "ingen/types.hpp"

namespace Raul {
class Path;
}

namespace ingen {

class Atom;
class URI;

/** Abstract interface for Ingen servers and clients.
 *
 * @ingroup Ingen
 */
class INGEN_API Interface
{
public:
	using result_type = void;

	Interface() : _seq(0) {}

	virtual ~Interface() = default;

	virtual URI uri() const = 0;

	virtual SPtr<Interface> respondee() const { return SPtr<Interface>(); }

	virtual void set_respondee(SPtr<Interface> respondee) {}

	virtual void message(const Message& msg) = 0;

	/** @name Convenience API
	 * @{
	 */

	inline void operator()(const Message& msg) { message(msg); }

	inline void set_response_id(int32_t id) { _seq = id; }

	inline void bundle_begin() { message(BundleBegin{_seq++}); }
	inline void bundle_end()   { message(BundleEnd{_seq++}); }

	inline void put(const URI&        uri,
	                const Properties& properties,
	                Resource::Graph   ctx = Resource::Graph::DEFAULT)
	{
		message(Put{_seq++, uri, properties, ctx});
	}

	inline void delta(const URI&        uri,
	                  const Properties& remove,
	                  const Properties& add,
	                  Resource::Graph   ctx = Resource::Graph::DEFAULT)
	{
		message(Delta{_seq++, uri, remove, add, ctx});
	}

	inline void copy(const URI& old_uri, const URI& new_uri)
	{
		message(Copy{_seq++, old_uri, new_uri});
	}

	inline void move(const Raul::Path& old_path, const Raul::Path& new_path)
	{
		message(Move{_seq++, old_path, new_path});
	}

	inline void del(const URI& uri) { message(Del{_seq++, uri}); }

	inline void connect(const Raul::Path& tail, const Raul::Path& head)
	{
		message(Connect{_seq++, tail, head});
	}

	inline void disconnect(const Raul::Path& tail, const Raul::Path& head)
	{
		message(Disconnect{_seq++, tail, head});
	}

	inline void disconnect_all(const Raul::Path& graph, const Raul::Path& path)
	{
		message(DisconnectAll{_seq++, graph, path});
	}

	inline void set_property(const URI&      subject,
	                         const URI&      predicate,
	                         const Atom&     value,
	                         Resource::Graph ctx = Resource::Graph::DEFAULT)
	{
		message(SetProperty{_seq++, subject, predicate, value, ctx});
	}

	inline void undo() { message(Undo{_seq++}); }

	inline void redo() { message(Redo{_seq++}); }

	inline void get(const URI& uri) { message(Get{_seq++, uri}); }

	inline void response(int32_t id, Status status, const std::string& subject)
	{
		message(Response{id, status, subject});
	}

	inline void error(const std::string& error_message)
	{
		message(Error{_seq++, error_message});
	}

	/** @} */

private:
	int32_t _seq;
};

} // namespace ingen

#endif // INGEN_INTERFACE_HPP
