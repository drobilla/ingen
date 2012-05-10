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

#ifndef INGEN_ENGINE_EVENTWRITER_HPP
#define INGEN_ENGINE_EVENTWRITER_HPP

#include <inttypes.h>
#include <memory>
#include <string>

#include "ingen/Interface.hpp"
#include "ingen/Resource.hpp"
#include "raul/SharedPtr.hpp"

#include "types.hpp"

namespace Ingen {
namespace Server {

class Engine;

/** An Interface that creates and enqueues Events for the Engine to execute.
 */
class EventWriter : public Interface
{
public:
	explicit EventWriter(Engine& engine);
	virtual ~EventWriter();

	Raul::URI uri() const { return "http://drobilla.net/ns/ingen#internal"; }

	virtual SharedPtr<Interface> respondee() const {
		return _respondee;
	}

	virtual void set_respondee(SharedPtr<Interface> respondee) {
		_respondee = respondee;
	}

	virtual void set_response_id(int32_t id);

	virtual void bundle_begin() {}

	virtual void bundle_end() {}

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
	Engine&              _engine;
	SharedPtr<Interface> _respondee;
	int32_t              _request_id;

private:
	SampleCount now() const;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_EVENTWRITER_HPP
