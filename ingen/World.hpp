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

#ifndef INGEN_WORLD_HPP
#define INGEN_WORLD_HPP

#include <string>

#include "ingen/types.hpp"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "raul/Noncopyable.hpp"
#include "raul/URI.hpp"

typedef struct LilvWorldImpl LilvWorld;

namespace Sord { class World; }

namespace Ingen {

class Configuration;
class EngineBase;
class Forge;
class Interface;
class LV2Features;
class Log;
class Parser;
class Serialiser;
class Store;
class URIMap;
class URIs;

/** The "world" all Ingen modules share.
 *
 * This is the root to which all components of Ingen are connected.  It
 * contains all necessary shared data (including the world for libraries like
 * Sord and Lilv) and holds references to components.
 *
 * Some functionality in Ingen is implemented in dynamically loaded modules,
 * which are loaded using this interface.  When loaded, those modules add
 * facilities to the World which can then be used throughout the code.
 *
 * The world is used in any process which uses the Ingen as a library, both
 * client and server (e.g. the world may not actually contain an Engine, since
 * it maybe running in another process or even on a different machine).
 *
 * @ingroup IngenShared
 */
class World : public Raul::Noncopyable {
public:
	/** Construct a new Ingen world.
	 * @param argc Argument count (as in C main())
	 * @param argv Argument vector (as in C main())
	 * @param map LV2 URID map implementation, or NULL to use internal.
	 * @param unmap LV2 URID unmap implementation, or NULL to use internal.
	 * @param log LV2 log implementation, or NULL to use internal.
	 */
	World(int&            argc,
	      char**&         argv,
	      LV2_URID_Map*   map,
	      LV2_URID_Unmap* unmap,
	      LV2_Log_Log*    log);

	virtual ~World();

	/** Load an Ingen module by name (e.g. "server", "gui", etc.)
	 * @return True on success.
	 */
	virtual bool load_module(const char* name);

	/** Run a loaded module (modules that "run" only, e.g. gui).
	 * @return True on success.
	 */
	virtual bool run_module(const char* name);

	/** A function to create a new remote Interface. */
	typedef SPtr<Interface> (*InterfaceFactory)(
		World*           world,
		const Raul::URI& engine_uri,
		SPtr<Interface>  respondee);

	/** Register an InterfaceFactory (for module implementations). */
	virtual void add_interface_factory(const std::string& scheme,
	                                   InterfaceFactory   factory);

	/** Return a new Interface to control a server.
	 * @param engine_uri The URI of the possibly remote server to control.
	 * @param respondee The Interface that will receive responses to commands
	 *                  and broadcasts, if applicable.
	 */
	virtual SPtr<Interface> new_interface(const Raul::URI& engine_uri,
	                                      SPtr<Interface>  respondee);

	/** Run a script. */
	virtual bool run(const std::string& mime_type,
	                 const std::string& filename);

	virtual void set_engine(SPtr<EngineBase> e);
	virtual void set_interface(SPtr<Interface> e);
	virtual void set_store(SPtr<Store> s);

	virtual SPtr<EngineBase> engine();
	virtual SPtr<Interface>  interface();
	virtual SPtr<Parser>     parser();
	virtual SPtr<Serialiser> serialiser();
	virtual SPtr<Store>      store();

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

	virtual Log& log();

private:
	class Impl;

	Impl* _impl;
};

}  // namespace Ingen

#endif  // INGEN_WORLD_HPP
