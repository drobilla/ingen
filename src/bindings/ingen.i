%include "stl.i"
%module(directors="1") ingen
%{
#include "common/interface/CommonInterface.hpp"
#include "common/interface/ClientInterface.hpp"
#include "common/interface/EngineInterface.hpp"
#include "module/World.hpp"
#include "ingen_bindings.hpp"
#include "Client.hpp"
%}

/*%ignore Ingen::Shared::EngineInterface;*/

%include "../common/interface/CommonInterface.hpp"
%include "../common/interface/ClientInterface.hpp"
%include "../common/interface/EngineInterface.hpp"
%include "../module/World.hpp"
//%include "../module/module.h"
%include "ingen_bindings.hpp"

// generate directors for all classes that have virtual methods
%feature("director");         
%feature("director") Ingen::Shared::ClientInterface;
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

    /*SLV2World slv2() { return $self->me->slv2_world; }*/
};

} }


%include "Client.hpp"

%feature("director") Client;


/*SharedPtr<Ingen::Shared::EngineInterface> engine() { return $self->me->engine; }*/

