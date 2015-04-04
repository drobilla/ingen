/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_BROADCASTER_HPP
#define INGEN_ENGINE_BROADCASTER_HPP

#include <atomic>
#include <list>
#include <mutex>
#include <set>
#include <string>

#include "ingen/Interface.hpp"
#include "ingen/types.hpp"

#include "BlockFactory.hpp"

namespace Ingen {
namespace Server {

/** Broadcaster for all clients.
 *
 * This is an Interface that forwards all messages to all registered
 * clients (for updating all clients on state changes in the engine).
 *
 * \ingroup engine
 */
class Broadcaster : public Interface
{
public:
	Broadcaster();
	~Broadcaster();

	void register_client(SPtr<Interface> client);
	bool unregister_client(SPtr<Interface> client);

	void set_broadcast(SPtr<Interface> client, bool broadcast);

	/** Ignore a client when broadcasting.
	 *
	 * This is used to prevent feeding back updates to the client that
	 * initiated a property set in the first place.
	 */
	void set_ignore_client(SPtr<Interface> client) { _ignore_client = client; }
	void clear_ignore_client()                     { _ignore_client.reset(); }

	/** Return true iff there are any clients with broadcasting enabled.
	 *
	 * This is used in the audio thread to decide whether or not notifications
	 * should be calculated and emitted.
	 */
	bool must_broadcast() const { return _must_broadcast; }

	/** A handle that represents a transfer of possibly several changes.
	 *
	 * This object going out of scope signifies the transfer is completed.
	 * This makes doing the right thing in recursive functions that send
	 * updates simple (e.g. Event::post_process()).
	 */
	class Transfer : public Raul::Noncopyable {
	public:
		explicit Transfer(Broadcaster& b) : broadcaster(b) {
			if (++broadcaster._bundle_depth == 1) {
				broadcaster.bundle_begin();
			}
		}
		~Transfer() {
			if (--broadcaster._bundle_depth == 0) {
				broadcaster.bundle_end();
			}
		}
		Broadcaster& broadcaster;
	};

	void send_plugins(const BlockFactory::Plugins& plugin_list);
	void send_plugins_to(Interface*, const BlockFactory::Plugins& plugin_list);

#define BROADCAST(msg, ...) \
	std::lock_guard<std::mutex> lock(_clients_mutex); \
	for (const auto& c : _clients) { \
		if (c != _ignore_client) { \
			c->msg(__VA_ARGS__); \
		} \
	} \

	void bundle_begin() { BROADCAST(bundle_begin); }
	void bundle_end()   { BROADCAST(bundle_end); }

	void put(const Raul::URI&            uri,
	         const Resource::Properties& properties,
	         Resource::Graph             ctx=Resource::Graph::DEFAULT) {
		BROADCAST(put, uri, properties);
	}

	void delta(const Raul::URI&            uri,
	           const Resource::Properties& remove,
	           const Resource::Properties& add) {
		BROADCAST(delta, uri, remove, add);
	}

	void copy(const Raul::Path& old_path,
	          const Raul::URI&  new_uri) {
		BROADCAST(copy, old_path, new_uri);
	}

	void move(const Raul::Path& old_path,
	          const Raul::Path& new_path) {
		BROADCAST(move, old_path, new_path);
	}

	void del(const Raul::URI& uri) {
		BROADCAST(del, uri);
	}

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
	                  const Atom&      value) {
		BROADCAST(set_property, subject, predicate, value);
	}

	Raul::URI uri() const { return Raul::URI("ingen:/broadcaster"); }

	void set_response_id(int32_t id) {} ///< N/A
	void get(const Raul::URI& uri) {} ///< N/A
	void response(int32_t id, Status status, const std::string& subject) {} ///< N/A

	void error(const std::string& msg) { BROADCAST(error, msg); }

private:
	friend class Transfer;

	typedef std::set<SPtr<Interface>> Clients;

	std::mutex                  _clients_mutex;
	Clients                     _clients;
	std::set< SPtr<Interface> > _broadcastees;
	std::atomic<bool>           _must_broadcast;
	unsigned                    _bundle_depth;
	SPtr<Interface>             _ignore_client;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_BROADCASTER_HPP
