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

/*%ignore Ingen::Shared::EngineInterface;*/

%include "../../include/ingen/CommonInterface.hpp"
%include "../../include/ingen/ClientInterface.hpp"
%include "../../include/ingen/EngineInterface.hpp"
%include "../../includemodule/World.hpp"
//%include "../module/module.h"
%include "ingen_bindings.hpp"

// generate directors for all classes that have virtual methods
%feature("director");         
%feature("director") Ingen::ClientInterface;
//%feature("director") Ingen::Shared::EngineInterface;

typedef Ingen::Shared::World World;
namespace Ingen { namespace Shared {
%extend World {
    World() {
        if (!Ingen::Shared::ingen_world) {
            fprintf(stderr, "ERROR: World uninitialized (running within Ingen?)\n");
            abort();
        } else {
            return Ingen::Shared::ingen_world;
        }
    }

    void iteration() {
        Ingen::Shared::script_iteration($self);
    }

    /*LILVWorld lilv() { return $self->me->lilv_world; }*/
};

} }


%include "Client.hpp"

%feature("director") Client;


/*SharedPtr<Ingen::Shared::EngineInterface> engine() { return $self->me->engine; }*/

