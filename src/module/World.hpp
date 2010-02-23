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
#include <glibmm/module.h>
#include "raul/Configuration.hpp"
#include "Module.hpp"

typedef struct _SLV2World* SLV2World;

namespace Redland { class World; }

namespace Ingen {

class Engine;

namespace Serialisation { class Serialiser; class Parser; }

namespace Shared {

class EngineInterface;
class Store;
class LV2Features;
class LV2URIMap;


/** The "world" all Ingen modules may share.
 *
 * All loaded components of Ingen, as well as things requiring shared access
 * and/or locking (e.g. Redland, SLV2).
 *
 * Ingen modules are shared libraries which modify the World when loaded
 * using World::load, e.g. loading the "ingen_serialisation" module will
 * set World::serialiser and World::parser to valid objects.
 */
struct World {
	World() : argc(0), argv(0), conf(0), rdf_world(0), slv2_world(0), lv2_features(0) {}

	bool load(const char* name);
	void unload_all();

	typedef SharedPtr<Ingen::Shared::EngineInterface> (*InterfaceFactory)(
			World* world, const std::string& engine_url);

	void add_interface_factory(const std::string& scheme, InterfaceFactory factory);
	SharedPtr<Ingen::Shared::EngineInterface> interface(const std::string& engine_url);

	bool run(const std::string& mime_type, const std::string& filename);

	int                  argc;
	char**               argv;
	Raul::Configuration* conf;

	Redland::World*      rdf_world;
    SLV2World            slv2_world;
	LV2Features*         lv2_features;

	boost::shared_ptr<LV2URIMap> uris;

    boost::shared_ptr<EngineInterface>           engine;
    boost::shared_ptr<Engine>                    local_engine;
    boost::shared_ptr<Serialisation::Serialiser> serialiser;
    boost::shared_ptr<Serialisation::Parser>     parser;
    boost::shared_ptr<Store>                     store;

private:
	typedef std::map< const std::string, boost::shared_ptr<Module> > Modules;
	Modules modules;

	typedef std::map<const std::string, InterfaceFactory> InterfaceFactories;
	InterfaceFactories interface_factories;

	typedef bool (*ScriptRunner)(World* world, const char* filename);
	typedef std::map<const std::string, ScriptRunner> ScriptRunners;
	ScriptRunners script_runners;
};


} // namespace Shared
} // namespace Ingen

#endif // INGEN_MODULE_WORLD_HPP

