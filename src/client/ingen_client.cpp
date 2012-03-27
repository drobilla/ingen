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

#include "ingen/shared/Module.hpp"
#include "ingen/shared/World.hpp"
#include "raul/SharedPtr.hpp"

#include "ingen_config.h"

using namespace Ingen;

struct IngenClientModule : public Ingen::Shared::Module {
	void load(Ingen::Shared::World* world) {
	}
};

extern "C" {

Ingen::Shared::Module*
ingen_module_load()
{
	return new IngenClientModule();
}

} // extern "C"

