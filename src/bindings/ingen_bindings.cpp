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

#include "python2.4/Python.h"
#include "ingen_bindings.hpp"
#include "server/Engine.hpp"
#include "ingen/World.hpp"

bool
run(Ingen::World* world, const char* filename)
{
	ingen_world = world;

	FILE* fd = fopen(filename, "r");
	if (fd) {
		info << "Executing script " << filename << endl;
		Py_Initialize();
		PyRun_SimpleFile(fd, filename);
		Py_Finalize();
		return true;
	} else {
		error << "Unable to open script " << filename << endl;
		return false;
	}
}

struct IngenBindingsModule : public Ingen::Module {
	void load(Ingen::World* world) {
		world->script_runners.insert(make_pair("application/x-python", &run));
	}
};

extern "C" {

Ingen::Module*
ingen_module_load()
{
	return new IngenBindingsModule();
}

void
script_iteration(Ingen::World* world)
{
	if (world->engine())
		world->engine()->main_iteration();
}

} // extern "C"
