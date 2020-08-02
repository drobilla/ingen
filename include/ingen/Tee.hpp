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

#ifndef INGEN_TEE_HPP
#define INGEN_TEE_HPP

#include "ingen/Interface.hpp"
#include "ingen/Message.hpp"

#include <cstddef>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace ingen {

/** Interface that forwards all calls to several sinks. */
class Tee : public Interface
{
public:
	using Sinks = std::vector<std::shared_ptr<Interface>>;

	explicit Tee(Sinks sinks) : _sinks(std::move(sinks)) {}

	std::shared_ptr<Interface> respondee() const override {
		return _sinks.front()->respondee();
	}

	void set_respondee(const std::shared_ptr<Interface>& respondee) override
	{
		_sinks.front()->set_respondee(respondee);
	}

	void message(const Message& message) override {
		std::lock_guard<std::mutex> lock(_sinks_mutex);
		for (const auto& s : _sinks) {
			s->message(message);
		}
	}

	URI uri() const override { return URI("ingen:/tee"); }

private:
	std::mutex _sinks_mutex;
	Sinks      _sinks;
};

} // namespace ingen

#endif // INGEN_TEE_HPP
