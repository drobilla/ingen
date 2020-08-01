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

#include "JackDriver.hpp"
#include "Engine.hpp"

#include "ingen/Atom.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/Log.hpp"
#include "ingen/Module.hpp"
#include "ingen/World.hpp"

#include <string>

using namespace ingen;

struct IngenJackModule : public ingen::Module {
	void load(ingen::World& world) override {
		server::Engine* const engine =
		    static_cast<server::Engine*>(world.engine().get());

		if (engine->driver()) {
			world.log().warn("Engine already has a driver\n");
			return;
		}

		auto*             driver      = new server::JackDriver(*engine);
		const Atom&       s           = world.conf().option("jack-server");
		const std::string server_name = s.is_valid() ? s.ptr<char>() : "";

		driver->attach(server_name,
		               world.conf().option("jack-name").ptr<char>(),
		               nullptr);

		engine->set_driver(SPtr<server::Driver>(driver));
	}
};

extern "C" {

ingen::Module*
ingen_module_load()
{
	return new IngenJackModule();
}

} // extern "C"
