/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "ingen/shared/Module.hpp"
#include "ingen/shared/World.hpp"
#include "raul/log.hpp"

#include "JackDriver.hpp"
#include "Engine.hpp"

using namespace std;
using namespace Ingen;

struct IngenJackModule : public Ingen::Shared::Module {
	void load(Ingen::Shared::World* world) {
		if (((Server::Engine*)world->local_engine().get())->driver()) {
			Raul::warn << "Engine already has a driver" << std::endl;
			return;
		}
			
		Server::JackDriver* driver = new Server::JackDriver(
			*(Server::Engine*)world->local_engine().get());
		driver->attach(world->conf()->option("jack-server").get_string(),
				world->conf()->option("jack-client").get_string(), NULL);
		((Server::Engine*)world->local_engine().get())->set_driver(
			SharedPtr<Server::Driver>(driver));
	}
};

extern "C" {

Ingen::Shared::Module*
ingen_module_load()
{
	return new IngenJackModule();
}

} // extern "C"
