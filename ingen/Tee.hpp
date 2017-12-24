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

#include <cstddef>
#include <mutex>
#include <set>
#include <utility>

#include "ingen/Interface.hpp"
#include "ingen/Message.hpp"
#include "ingen/types.hpp"
#include "raul/URI.hpp"

namespace Ingen {

/** Interface that forwards all calls to several sinks. */
class Tee : public Interface
{
public:
	typedef std::set< SPtr<Interface> > Sinks;

	explicit Tee(Sinks sinks) : _sinks(std::move(sinks)) {}

	SPtr<Interface> respondee() const override {
		return (*_sinks.begin())->respondee();
	}

	void set_respondee(SPtr<Interface> respondee) override {
		(*_sinks.begin())->set_respondee(respondee);
	}

	void message(const Message& message) override {
		std::lock_guard<std::mutex> lock(_sinks_mutex);
		for (const auto& s : _sinks) {
			s->message(message);
		}
	}

	Raul::URI uri() const override { return Raul::URI("ingen:/tee"); }

private:
	std::mutex _sinks_mutex;
	Sinks      _sinks;
};

} // namespace Ingen

#endif // INGEN_ENGINE_TEE_HPP
