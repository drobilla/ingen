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

#ifndef INGEN_ENGINE_BROADCASTER_HPP
#define INGEN_ENGINE_BROADCASTER_HPP

#include "BlockFactory.hpp"

#include "ingen/Interface.hpp"
#include "ingen/Message.hpp"
#include "ingen/URI.hpp"
#include "ingen/types.hpp"
#include "raul/Noncopyable.hpp"

#include <atomic>
#include <mutex>
#include <set>

namespace ingen {
namespace server {

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
	Broadcaster() = default;
	~Broadcaster() override;

	void register_client(const SPtr<Interface>& client);
	bool unregister_client(const SPtr<Interface>& client);

	void set_broadcast(const SPtr<Interface>& client, bool broadcast);

	/** Ignore a client when broadcasting.
	 *
	 * This is used to prevent feeding back updates to the client that
	 * initiated a property set in the first place.
	 */
	void set_ignore_client(const SPtr<Interface>& client)
	{
		_ignore_client = client;
	}

	void clear_ignore_client() { _ignore_client.reset(); }

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

	void send_plugins(const BlockFactory::Plugins& plugins);

	static void
	send_plugins_to(Interface*, const BlockFactory::Plugins& plugins);

	void message(const Message& msg) override {
		std::lock_guard<std::mutex> lock(_clients_mutex);
		for (const auto& c : _clients) {
			if (c != _ignore_client) {
				c->message(msg);
			}
		}
	}

	URI uri() const override { return URI("ingen:/broadcaster"); }

private:
	friend class Transfer;

	using Clients = std::set<SPtr<Interface>>;

	std::mutex                  _clients_mutex;
	Clients                     _clients;
	std::set< SPtr<Interface> > _broadcastees;
	std::atomic<bool>           _must_broadcast{false};
	unsigned                    _bundle_depth{0};
	SPtr<Interface>             _ignore_client;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_BROADCASTER_HPP
