/*
  This file is part of Ingen.
  Copyright 2018 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_QUEUEDINTERFACE_HPP
#define INGEN_ENGINE_QUEUEDINTERFACE_HPP

#include <mutex>
#include <vector>

#include "ingen/Interface.hpp"
#include "ingen/Message.hpp"

namespace ingen {

/** Stores all messages and emits them to a sink on demand.
 *
 * This can be used to make an interface thread-safe.
 */
class QueuedInterface : public Interface
{
public:
	explicit QueuedInterface(SPtr<Interface> sink) : _sink(std::move(sink)) {}

	URI uri() const override { return URI("ingen:/QueuedInterface"); }

	void message(const Message& message) override {
		std::lock_guard<std::mutex> lock(_mutex);
		_messages.emplace_back(message);
	}

	void emit() {
		std::vector<Message> messages;
		{
			std::lock_guard<std::mutex> lock(_mutex);
			_messages.swap(messages);
		}

		for (const auto& i : messages) {
			_sink->message(i);
		}
	}

	const SPtr<Interface>& sink() const { return _sink; }

private:
	std::mutex           _mutex;
	SPtr<Interface>      _sink;
	std::vector<Message> _messages;
};

} // namespace ingen

#endif // INGEN_ENGINE_QUEUEDINTERFACE_HPP
