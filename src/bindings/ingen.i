%include "stl.i"
%module(directors="1") ingen
%{
#include "ingen/CommonInterface.hpp"
#include "ingen/ClientInterface.hpp"
#include "ingen/EngineInterface.hpp"
#include "module/World.hpp"
#include "ingen_bindings.hpp"
#include "Client.hpp"
%}

%include "../../ingen/CommonInterface.hpp"
%include "../../ingen/ClientInterface.hpp"
%include "../../ingen/EngineInterface.hpp"
%include "../../includemodule/World.hpp"
//%include "../module/module.h"
%include "ingen_bindings.hpp"

// generate directors for all classes that have virtual methods
%feature("director");         
%feature("director") Ingen::ClientInterface;

typedef Ingen::World World;
namespace Ingen {
%extend World {
    World() {
        if (!Ingen::ingen_world) {
            fprintf(stderr, "ERROR: World uninitialized (running within Ingen?)\n");
            abort();
        } else {
            return Ingen::ingen_world;
        }
    }

    void iteration() {
        Ingen::script_iteration($self);
    }

    /*LILVWorld lilv() { return $self->me->lilv_world; }*/
};

}


%include "Client.hpp"

%feature("director") Client;
