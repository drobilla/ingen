#include <iostream>
#include "python2.4/Python.h"
#include "ingen_bindings.hpp"
#include "../libs/engine/Engine.hpp"

using namespace std;

namespace Ingen {
namespace Shared {

Ingen::Shared::World* ingen_world = NULL;

extern "C" {

bool
run(Ingen::Shared::World* world, const char* filename)
{
    ingen_world = world;

    FILE* fd = fopen(filename, "r");
    if (fd) {
        cerr << "EXECUTING " << filename << endl;
        Py_Initialize();
        PyRun_SimpleFile(fd, filename);
        Py_Finalize();
        return true;
    } else {
        cerr << "UNABLE TO OPEN FILE " << filename << endl;
        return false;
    }
}


void
script_iteration(Ingen::Shared::World* world) {
    if (world->local_engine)
        world->local_engine->main_iteration();
}


}

} // namespace Shared
} // namespace Ingen
