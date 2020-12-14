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

// IWYU pragma: no_include "ingen/Atom.hpp"

#include "Engine.hpp"
#include "util.hpp"

#include "ingen/Module.hpp"
#include "ingen/World.hpp"

#include <memory>

namespace ingen {

struct EngineModule : public Module {
	void load(World& world) override {
		server::set_denormal_flags(world.log());
		auto engine = std::make_shared<server::Engine>(world);
		world.set_engine(engine);
		if (!world.interface()) {
			world.set_interface(engine->interface());
		}
	}
};

} // namespace ingen

extern "C" {

ingen::Module*
ingen_module_load()
{
	return new ingen::EngineModule();
}

} // extern "C"
