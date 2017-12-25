/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

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

#include "ingen/Atom.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/Log.hpp"
#include "ingen/Module.hpp"
#include "ingen/World.hpp"

#include "PortAudioDriver.hpp"
#include "Engine.hpp"

using namespace Ingen;

struct IngenPortAudioModule : public Ingen::Module {
	void load(Ingen::World* world) {
		if (((Server::Engine*)world->engine().get())->driver()) {
			world->log().warn("Engine already has a driver\n");
			return;
		}

		Server::PortAudioDriver* driver = new Server::PortAudioDriver(
			*(Server::Engine*)world->engine().get());
		driver->attach();
		((Server::Engine*)world->engine().get())->set_driver(
			SPtr<Server::Driver>(driver));
	}
};

extern "C" {

Ingen::Module*
ingen_module_load()
{
	return new IngenPortAudioModule();
}

} // extern "C"
