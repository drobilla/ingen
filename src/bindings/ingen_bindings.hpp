#ifndef INGEN_BINDINGS_HPP
#define INGEN_BINDINGS_HPP

namespace Ingen {
namespace Shared {


class World;
extern World* ingen_world;

extern "C" {
    bool run(World* world, const char* filename);
    void script_iteration(World* world);
}

} // namespace Shared
} // namespace Ingen

#endif // INGEN_BINDINGS_HPP
