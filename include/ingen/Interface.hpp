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

#include "ingen/Message.hpp"
#include "ingen/Properties.hpp"
#include "ingen/Resource.hpp"
#include "ingen/Status.hpp"
#include "ingen/ingen.h"

#include <cstdint>
#include <memory>
#include <string>

namespace raul {
class Path;
} // namespace raul

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

	Interface() = default;

	virtual ~Interface() = default;

	virtual URI uri() const = 0;

	virtual std::shared_ptr<Interface> respondee() const { return nullptr; }

	virtual void set_respondee(const std::shared_ptr<Interface>& respondee) {}

	virtual void message(const Message& msg) = 0;

	/** @name Convenience API
	 * @{
	 */

	void operator()(const Message& msg) { message(msg); }

	void set_response_id(int32_t id) { _seq = id; }

	void bundle_begin() { message(BundleBegin{_seq++}); }
	void bundle_end()   { message(BundleEnd{_seq++}); }

	void put(const URI&        uri,
	         const Properties& properties,
	         Resource::Graph   ctx = Resource::Graph::DEFAULT)
	{
		message(Put{_seq++, uri, properties, ctx});
	}

	void delta(const URI&        uri,
	           const Properties& remove,
	           const Properties& add,
	           Resource::Graph   ctx = Resource::Graph::DEFAULT)
	{
		message(Delta{_seq++, uri, remove, add, ctx});
	}

	void copy(const URI& old_uri, const URI& new_uri)
	{
		message(Copy{_seq++, old_uri, new_uri});
	}

	void move(const raul::Path& old_path, const raul::Path& new_path)
	{
		message(Move{_seq++, old_path, new_path});
	}

	void del(const URI& uri) { message(Del{_seq++, uri}); }

	void connect(const raul::Path& tail, const raul::Path& head)
	{
		message(Connect{_seq++, tail, head});
	}

	void disconnect(const raul::Path& tail, const raul::Path& head)
	{
		message(Disconnect{_seq++, tail, head});
	}

	void disconnect_all(const raul::Path& graph, const raul::Path& path)
	{
		message(DisconnectAll{_seq++, graph, path});
	}

	void set_property(const URI&      subject,
	                  const URI&      predicate,
	                  const Atom&     value,
	                  Resource::Graph ctx = Resource::Graph::DEFAULT)
	{
		message(SetProperty{_seq++, subject, predicate, value, ctx});
	}

	void undo() { message(Undo{_seq++}); }

	void redo() { message(Redo{_seq++}); }

	void get(const URI& uri) { message(Get{_seq++, uri}); }

	void response(int32_t id, Status status, const std::string& subject)
	{
		message(Response{id, status, subject});
	}

	void error(const std::string& error_message)
	{
		message(Error{_seq++, error_message});
	}

	/** @} */

private:
	int32_t _seq = 0;
};

} // namespace ingen

#endif // INGEN_INTERFACE_HPP
