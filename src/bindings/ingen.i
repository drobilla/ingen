%include "stl.i"
%module ingen
%{
#include "../common/interface/ClientInterface.hpp"
#include "../common/interface/EngineInterface.hpp"
#include "../libs/module/World.hpp"
/*#include "../libs/module/module.h"*/
#include "ingen_bindings.hpp"

namespace Ingen { namespace Shared {
    class World;
} }
%}

/*%ignore Ingen::Shared::EngineInterface;*/

%include "../common/interface/ClientInterface.hpp"
%include "../common/interface/EngineInterface.hpp"
/*%include "../libs/module/World.hpp"
%include "../libs/module/module.h"*/
%include "../libs/module/World.hpp"
//%include "../libs/module/module.h"
%include "ingen_bindings.hpp"

using namespace Ingen::Shared;
namespace Ingen { namespace Shared {
    class World;
} }
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
    /*SLV2World slv2() { return $self->me->slv2_world; }*/
};
} }

/*SharedPtr<Ingen::Shared::EngineInterface> engine() { return $self->me->engine; }*/

