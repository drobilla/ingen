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

#ifndef INGEN_SHARED_WORLD_HPP
#define INGEN_SHARED_WORLD_HPP

#include <string>

#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "raul/Noncopyable.hpp"
#include "raul/SharedPtr.hpp"

typedef struct LilvWorldImpl LilvWorld;

namespace Sord { class World; }

namespace Ingen {

class EngineBase;
class Forge;
class Interface;

namespace Serialisation {
class Parser;
class Serialiser;
}

namespace Shared {

class Configuration;
class LV2Features;
class Store;
class URIMap;
class URIs;

/** The "world" all Ingen modules share.
 *
 * This is the root to which all components of Ingen are connected.  It
 * contains all necessary shared data (including the world for libraries like
 * Sord and Lilv) and holds references to components.
 *
 * Most functionality in Ingen is implemented in dynamically loaded modules,
 * which are loaded using this interface.  When loaded, those modules add
 * facilities to the World which can then be used throughout the code.  For
 * example loading the "ingen_serialisation" module will set World::serialiser
 * and World::parser to valid objects.
 *
 * The world is used in any process which uses the Ingen as a library, both
 * client and server (e.g. the world may not actually contain an Engine, since
 * it maybe running in another process or even on a different machine).
 */
class World : public Raul::Noncopyable {
public:
	/** Construct a new Ingen world.
	 * @param argc Argument count (as in C main())
	 * @param argv Argument vector (as in C main())
	 * @param map LV2 URID map implementation, or NULL to use internal.
	 * @param unmap LV2 URID unmap implementation, or NULL to use internal.
	 */
	World(int&            argc,
	      char**&         argv,
	      LV2_URID_Map*   map,
	      LV2_URID_Unmap* unmap);

	virtual ~World();

	/** Load an Ingen module by name (e.g. "server", "gui", etc.)
	 * @return True on success.
	 */
	virtual bool load_module(const char* name);

	/** Run a loaded module (modules that "run" only, e.g. gui).
	 * @return True on success.
	 */
	virtual bool run_module(const char* name);

	/** Unload all loaded Ingen modules. */
	virtual void unload_modules();

	/** A function to create a new remote Interface. */
	typedef SharedPtr<Interface> (*InterfaceFactory)(
		World*               world,
		const std::string&   engine_url,
		SharedPtr<Interface> respondee);

	/** Register an InterfaceFactory (for module implementations). */
	virtual void add_interface_factory(const std::string& scheme,
	                                   InterfaceFactory   factory);

	/** Return a new Interface to control the server at @p engine_url.
	 * @param respondee The Interface that will receive responses to commands
	 *                  and broadcasts, if applicable.
	 */
	virtual SharedPtr<Interface> new_interface(
		const std::string&   engine_url,
		SharedPtr<Interface> respondee);

	/** Run a script. */
	virtual bool run(const std::string& mime_type,
	                 const std::string& filename);

	virtual void set_engine(SharedPtr<EngineBase> e);
	virtual void set_interface(SharedPtr<Interface> e);
	virtual void set_parser(SharedPtr<Serialisation::Parser> p);
	virtual void set_serialiser(SharedPtr<Serialisation::Serialiser> s);
	virtual void set_store(SharedPtr<Store> s);

	virtual SharedPtr<EngineBase>                engine();
	virtual SharedPtr<Interface>                 interface();
	virtual SharedPtr<Serialisation::Parser>     parser();
	virtual SharedPtr<Serialisation::Serialiser> serialiser();
	virtual SharedPtr<Store>                     store();

	virtual int&           argc();
	virtual char**&        argv();
	virtual Configuration& conf();

	virtual Sord::World* rdf_world();
	virtual LilvWorld*   lilv_world();

	virtual LV2Features&  lv2_features();
	virtual Ingen::Forge& forge();
	virtual URIMap&       uri_map();
	virtual URIs&         uris();

	virtual void        set_jack_uuid(const std::string& uuid);
	virtual std::string jack_uuid();

private:
	class Impl;

	Impl* _impl;
};

}  // namespace Shared
}  // namespace Ingen

#endif  // INGEN_SHARED_WORLD_HPP
