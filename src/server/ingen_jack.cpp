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

#include <string>

#include "ingen/Configuration.hpp"
#include "ingen/Module.hpp"
#include "ingen/World.hpp"
#include "ingen/Log.hpp"
#include "raul/Configuration.hpp"

#include "JackDriver.hpp"
#include "Engine.hpp"

using namespace std;
using namespace Ingen;

struct IngenJackModule : public Ingen::Module {
	void load(Ingen::World* world) {
		if (((Server::Engine*)world->engine().get())->driver()) {
			world->log().warn("Engine already has a driver\n");
			return;
		}

		Server::JackDriver* driver = new Server::JackDriver(
			*(Server::Engine*)world->engine().get());
		const Raul::Configuration::Value& s = world->conf().option("jack-server");
		const std::string server_name = s.is_valid() ? s.get_string() : "";
		driver->attach(server_name,
		               world->conf().option("jack-client").get_string(),
		               NULL);
		((Server::Engine*)world->engine().get())->set_driver(
			SharedPtr<Server::Driver>(driver));
	}
};

extern "C" {

Ingen::Module*
ingen_module_load()
{
	return new IngenJackModule();
}

} // extern "C"
