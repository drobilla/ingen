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

#include "ingen/Module.hpp"
#include "ingen/World.hpp"
#include "Engine.hpp"
#include "EventWriter.hpp"
#include "util.hpp"

using namespace ingen;

struct IngenEngineModule : public ingen::Module {
	void load(ingen::World* world) override {
		server::set_denormal_flags(world->log());
		SPtr<server::Engine> engine(new server::Engine(world));
		world->set_engine(engine);
		if (!world->interface()) {
			world->set_interface(engine->interface());
		}
	}
};

extern "C" {

ingen::Module*
ingen_module_load()
{
	return new IngenEngineModule();
}

} // extern "C"
