#include "python2.4/Python.h"
#include "raul/log.hpp"
#include "ingen_bindings.hpp"
#include "engine/Engine.hpp"
#include "module/World.hpp"

bool
run(Ingen::Shared::World* world, const char* filename)
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

struct IngenBindingsModule : public Ingen::Shared::Module {
	void load(Ingen::Shared::World* world) {
		world->script_runners.insert(make_pair("application/x-python", &run));
	}
};

static IngenBindingsModule* module = NULL;

extern "C" {

Ingen::Shared::Module*
ingen_module_load() {
	if (!module)
		module = new IngenBindingsModule();

	return module;
}

void
script_iteration(Ingen::Shared::World* world)
{
	if (world->local_engine())
		world->local_engine()->main_iteration();
}

} // extern "C"
