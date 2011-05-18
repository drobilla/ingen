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

#include "shared/Module.hpp"
#include "shared/World.hpp"
#include "OSCEngineReceiver.hpp"
#include "Engine.hpp"

using namespace std;
using namespace Ingen;

struct IngenOSCModule : public Ingen::Shared::Module {
	void load(Ingen::Shared::World* world) {
		Server::Engine* engine = (Server::Engine*)world->local_engine().get();
		SharedPtr<Server::OSCEngineReceiver> interface(
			new Server::OSCEngineReceiver(
				*engine,
				world->conf()->option("engine-port").get_int32()));
		engine->add_event_source(interface);
	}
};

extern "C" {

Ingen::Shared::Module*
ingen_module_load()
{
	return new IngenOSCModule();
}

} // extern "C"
