%include "stl.i"
%module ingen
%{
#include "../common/interface/ClientInterface.hpp"
#include "../common/interface/EngineInterface.hpp"
#include "../libs/module/World.hpp"
#include "../libs/module/module.h"

namespace Ingen { namespace Shared {
    class World;
} }
typedef Ingen::Shared::World World;
/*struct World {
    World() { me = Ingen::Shared::get_world(); }
    Ingen::Shared::World* me;
};*/
%}

/*%ignore Ingen::Shared::EngineInterface;*/

%include "../common/interface/ClientInterface.hpp"
%include "../common/interface/EngineInterface.hpp"
/*%include "../libs/module/World.hpp"
%include "../libs/module/module.h"*/
%include "../libs/module/module.h"

using namespace Ingen::Shared;
namespace Ingen { namespace Shared {
    class World;
} }
%typedef Ingen::Shared::World World;
/*struct World {};*/
%extend World {
    World() { return Ingen::Shared::get_world(); }
    /*SLV2World slv2() { return $self->me->slv2_world; }*/

/*SharedPtr<Ingen::Shared::EngineInterface> engine() { return $self->me->engine; }*/
};

