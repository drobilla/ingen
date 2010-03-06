/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef INGEN_MODULE_WORLD_HPP
#define INGEN_MODULE_WORLD_HPP

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <glibmm/module.h>
#include "ingen-config.h"
#include "raul/Configuration.hpp"
#include "raul/SharedPtr.hpp"
#include "Module.hpp"
#include "module/ingen_module.hpp"

#ifdef HAVE_SLV2
typedef struct _SLV2World* SLV2World;
#endif

namespace Redland { class World; }

namespace Ingen {

class Engine;

namespace Serialisation { class Serialiser; class Parser; }

namespace Shared {

class EngineInterface;
class Store;
class LV2Features;
class LV2URIMap;
struct WorldImpl;


/** The "world" all Ingen modules may share.
 *
 * All loaded components of Ingen, as well as things requiring shared access
 * and/or locking (e.g. Redland, SLV2).
 *
 * Ingen modules are shared libraries which modify the World when loaded
 * using World::load, e.g. loading the "ingen_serialisation" module will
 * set World::serialiser and World::parser to valid objects.
 */
class World : public boost::noncopyable {
	friend Ingen::Shared::World* ::ingen_world_new(Raul::Configuration*, int&, char**&);
	World(Raul::Configuration* conf, int& argc, char**& argv);

	friend void ::ingen_world_free(Ingen::Shared::World* world);
	virtual ~World();

	WorldImpl* _impl;

public:
	virtual bool load(const char* name);
	virtual void unload_all();

	typedef SharedPtr<Ingen::Shared::EngineInterface> (*InterfaceFactory)(
			World* world, const std::string& engine_url);

	virtual void add_interface_factory(const std::string& scheme, InterfaceFactory factory);
	virtual SharedPtr<Ingen::Shared::EngineInterface> interface(const std::string& engine_url);

	virtual bool run(const std::string& mime_type, const std::string& filename);

	virtual void set_local_engine(SharedPtr<Engine> e);
	virtual void set_engine(SharedPtr<EngineInterface> e);
	virtual void set_serialiser(SharedPtr<Serialisation::Serialiser> s);
	virtual void set_parser(SharedPtr<Serialisation::Parser> p);
	virtual void set_store(SharedPtr<Store> s);

	virtual SharedPtr<Engine>                    local_engine();
	virtual SharedPtr<EngineInterface>           engine();
	virtual SharedPtr<Serialisation::Serialiser> serialiser();
	virtual SharedPtr<Serialisation::Parser>     parser();
	virtual SharedPtr<Store>                     store();

	virtual Redland::World*      rdf_world();
	virtual SharedPtr<LV2URIMap> uris();

	virtual int&    argc();
	virtual char**& argv();

	virtual Raul::Configuration* conf();
	virtual void set_conf(Raul::Configuration* c);

	virtual LV2Features* lv2_features();

#ifdef HAVE_SLV2
	virtual SLV2World slv2_world();
#endif
};


} // namespace Shared
} // namespace Ingen

#endif // INGEN_MODULE_WORLD_HPP

