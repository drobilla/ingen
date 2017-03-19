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

#ifndef INGEN_ENGINE_TEE_HPP
#define INGEN_ENGINE_TEE_HPP

#include <mutex>
#include <set>
#include <string>

#include "ingen/Interface.hpp"
#include "ingen/types.hpp"

namespace Ingen {

/** Interface that forwards all calls to several sinks. */
class Tee : public Interface
{
public:
	typedef std::set< SPtr<Interface> > Sinks;

	Tee(const Sinks& sinks = {})
		: _sinks(sinks)
	{}

	void add_sink(SPtr<Interface> sink) {
		std::lock_guard<std::mutex> lock(_sinks_mutex);
		_sinks.insert(sink);
	}

	bool remove_sink(SPtr<Interface> sink) {
		std::lock_guard<std::mutex> lock(_sinks_mutex);
		return (_sinks.erase(sink) > 0);
	}

	virtual SPtr<Interface> respondee() const {
		return (*_sinks.begin())->respondee();
	}

	virtual void set_respondee(SPtr<Interface> respondee) {
		(*_sinks.begin())->set_respondee(respondee);
	}

#define BROADCAST(msg, ...) \
	std::lock_guard<std::mutex> lock(_sinks_mutex); \
	for (const auto& s : _sinks) { \
		s->msg(__VA_ARGS__); \
	}

	void bundle_begin() { BROADCAST(bundle_begin); }
	void bundle_end()   { BROADCAST(bundle_end); }

	void put(const Raul::URI&  uri,
	         const Properties& properties,
	         Resource::Graph   ctx = Resource::Graph::DEFAULT) {
		BROADCAST(put, uri, properties, ctx);
	}

	void delta(const Raul::URI&  uri,
	           const Properties& remove,
	           const Properties& add,
	           Resource::Graph   ctx = Resource::Graph::DEFAULT) {
		BROADCAST(delta, uri, remove, add, ctx);
	}

	void copy(const Raul::URI& old_uri,
	          const Raul::URI& new_uri) {
		BROADCAST(copy, old_uri, new_uri);
	}

	void move(const Raul::Path& old_path,
	          const Raul::Path& new_path) {
		BROADCAST(move, old_path, new_path);
	}

	void del(const Raul::URI& uri) { BROADCAST(del, uri); }

	void connect(const Raul::Path& tail,
	             const Raul::Path& head) {
		BROADCAST(connect, tail, head);
	}

	void disconnect(const Raul::Path& tail,
	                const Raul::Path& head) {
		BROADCAST(disconnect, tail, head);
	}

	void disconnect_all(const Raul::Path& graph,
	                    const Raul::Path& path) {
		BROADCAST(disconnect_all, graph, path);
	}

	void set_property(const Raul::URI& subject,
	                  const Raul::URI& predicate,
	                  const Atom&      value,
	                  Resource::Graph  ctx = Resource::Graph::DEFAULT) {
		BROADCAST(set_property, subject, predicate, value, ctx);
	}

	void undo() { BROADCAST(undo); }
	void redo() { BROADCAST(redo); }

	void set_response_id(int32_t id) { BROADCAST(set_response_id, id); }

	void get(const Raul::URI& uri) { BROADCAST(get, uri); }

	void response(int32_t id, Status status, const std::string& subject) {
		BROADCAST(response, id, status, subject);
	}

	void error(const std::string& msg) { BROADCAST(error, msg); }

#undef BROADCAST

	Raul::URI    uri()   const { return Raul::URI("ingen:/tee"); }
	const Sinks& sinks() const { return _sinks; }
	Sinks&       sinks()       { return _sinks; }

private:
	std::mutex _sinks_mutex;
	Sinks      _sinks;
};

} // namespace Ingen

#endif // INGEN_ENGINE_TEE_HPP
