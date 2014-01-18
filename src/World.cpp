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

#include <map>
#include <string>

#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <glibmm/module.h>

#include "ingen/Configuration.hpp"
#include "ingen/EngineBase.hpp"
#include "ingen/LV2Features.hpp"
#include "ingen/Log.hpp"
#include "ingen/Module.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "ingen/ingen.h"
#include "ingen/runtime_paths.hpp"
#include "lilv/lilv.h"
#include "sord/sordmm.hpp"

using namespace std;

namespace Ingen {

class EngineBase;
class Interface;
class Store;

namespace Serialisation {
class Parser;
class Serialiser;
}

/** Load a dynamic module from the default path.
 *
 * This will check in the directories specified in the environment variable
 * INGEN_MODULE_PATH (typical colon delimited format), then the default module
 * installation directory (ie /usr/local/lib/ingen), in that order.
 *
 * \param name The base name of the module, e.g. "ingen_serialisation"
 */
static Glib::Module*
ingen_load_module(Log& log, const string& name)
{
	Glib::Module* module = NULL;

	// Search INGEN_MODULE_PATH first
	bool module_path_found;
	string module_path = Glib::getenv("INGEN_MODULE_PATH", module_path_found);
	if (module_path_found) {
		string dir;
		istringstream iss(module_path);
		while (getline(iss, dir, G_SEARCHPATH_SEPARATOR)) {
			string filename = Ingen::module_path(name, dir);
			if (Glib::file_test(filename, Glib::FILE_TEST_EXISTS)) {
				module = new Glib::Module(filename);
				if (*module) {
					return module;
				} else {
					log.error(Glib::Module::get_last_error());
				}
			}
		}
	}

	// Try default directory if not found
	module = new Glib::Module(Ingen::module_path(name));

	if (*module) {
		return module;
	} else if (!module_path_found) {
		log.error(fmt("Unable to find %1% (%2%)\n")
		          % name % Glib::Module::get_last_error());
		return NULL;
	} else {
		log.error(fmt("Unable to load %1% from %2% (%3%)\n")
		          % name % module_path % Glib::Module::get_last_error());
		return NULL;
	}
}

class World::Impl {
public:
	Impl(int&            a_argc,
	     char**&         a_argv,
	     LV2_URID_Map*   map,
	     LV2_URID_Unmap* unmap,
	     LV2_Log_Log*    lv2_log)
		: argc(a_argc)
		, argv(a_argv)
		, lv2_features(NULL)
		, rdf_world(new Sord::World())
		, uri_map(new URIMap(log, map, unmap))
		, forge(new Forge(*uri_map))
		, uris(new URIs(*forge, uri_map))
		, log(lv2_log, *uris)
		, conf(*forge)
		, lilv_world(lilv_world_new())
	{
		// Parse default configuration files
		std::list<std::string> files = conf.load_default("ingen", "options.ttl");
		for (const auto& f : files) {
			log.info(fmt("Loaded configuration %1%\n") % f);
		}

		// Parse command line options, overriding configuration file values
		conf.parse(argc, argv);

		lv2_features = new LV2Features();
		lv2_features->add_feature(uri_map->urid_map_feature());
		lv2_features->add_feature(uri_map->urid_unmap_feature());
		lilv_world_load_all(lilv_world);

		// Set up RDF namespaces
		rdf_world->add_prefix("atom",  "http://lv2plug.in/ns/ext/atom#");
		rdf_world->add_prefix("patch", "http://lv2plug.in/ns/ext/patch#");
		rdf_world->add_prefix("doap",  "http://usefulinc.com/ns/doap#");
		rdf_world->add_prefix("ingen", INGEN_NS);
		rdf_world->add_prefix("lv2",   "http://lv2plug.in/ns/lv2core#");
		rdf_world->add_prefix("midi",  "http://lv2plug.in/ns/ext/midi#");
		rdf_world->add_prefix("owl",   "http://www.w3.org/2002/07/owl#");
		rdf_world->add_prefix("rdf",   "http://www.w3.org/1999/02/22-rdf-syntax-ns#");
		rdf_world->add_prefix("rdfs",  "http://www.w3.org/2000/01/rdf-schema#");
		rdf_world->add_prefix("xsd",   "http://www.w3.org/2001/XMLSchema#");

		// Load internal 'plugin' information into lilv world
		LilvNode* rdf_type = lilv_new_uri(
			lilv_world, "http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
		LilvNode* ingen_Plugin = lilv_new_uri(
			lilv_world, INGEN__Plugin);
		LilvNodes* internals = lilv_world_find_nodes(
			lilv_world, NULL, rdf_type, ingen_Plugin);
		LILV_FOREACH(nodes, i, internals) {
			const LilvNode* internal = lilv_nodes_get(internals, i);
			lilv_world_load_resource(lilv_world, internal);
		}
		lilv_node_free(rdf_type);
		lilv_node_free(ingen_Plugin);
	}

	~Impl()
	{
		if (engine) {
			engine->quit();
		}

		// Delete module objects but save pointers to libraries
		typedef std::list<Glib::Module*> Libs;
		Libs libs;
		for (auto& m : modules) {
			libs.push_back(m.second->library);
			delete m.second;
		}

		serialiser.reset();
		parser.reset();
		interface.reset();
		engine.reset();
		store.reset();

		interface_factories.clear();
		script_runners.clear();

		delete rdf_world;
		delete lv2_features;
		delete uris;
		delete forge;
		delete uri_map;

		lilv_world_free(lilv_world);

		// Close module libraries
		for (auto& l : libs) {
			delete l;
		}
	}

	typedef std::map<const std::string, Module*> Modules;
	Modules modules;

	typedef std::map<const std::string, World::InterfaceFactory> InterfaceFactories;
	InterfaceFactories interface_factories;

	typedef bool (*ScriptRunner)(World* world, const char* filename);
	typedef std::map<const std::string, ScriptRunner> ScriptRunners;
	ScriptRunners script_runners;

	int&                            argc;
	char**&                         argv;
	LV2Features*                    lv2_features;
	Sord::World*                    rdf_world;
	URIMap*                         uri_map;
	Forge*                          forge;
	URIs*                           uris;
	LV2_Log_Log*                    lv2_log;
	Log                             log;
	Configuration                   conf;
	SPtr<Interface>                 interface;
	SPtr<EngineBase>                engine;
	SPtr<Serialisation::Serialiser> serialiser;
	SPtr<Serialisation::Parser>     parser;
	SPtr<Store>                     store;
	LilvWorld*                      lilv_world;
	std::string                     jack_uuid;
};

World::World(int&                 argc,
             char**&              argv,
             LV2_URID_Map*        map,
             LV2_URID_Unmap*      unmap,
             LV2_Log_Log*         log)
	: _impl(new Impl(argc, argv, map, unmap, log))
{
}

World::~World()
{
	delete _impl;
}

void World::set_engine(SPtr<EngineBase> e)                    { _impl->engine = e; }
void World::set_interface(SPtr<Interface> i)                  { _impl->interface = i; }
void World::set_parser(SPtr<Serialisation::Parser> p)         { _impl->parser = p; }
void World::set_serialiser(SPtr<Serialisation::Serialiser> s) { _impl->serialiser = s; }
void World::set_store(SPtr<Store> s)                          { _impl->store = s; }

SPtr<EngineBase>                World::engine()       { return _impl->engine; }
SPtr<Interface>                 World::interface()    { return _impl->interface; }
SPtr<Serialisation::Parser>     World::parser()       { return _impl->parser; }
SPtr<Serialisation::Serialiser> World::serialiser()   { return _impl->serialiser; }
SPtr<Store>                     World::store()        { return _impl->store; }

int&           World::argc() { return _impl->argc; }
char**&        World::argv() { return _impl->argv; }
Configuration& World::conf() { return _impl->conf; }
Log&           World::log()  { return _impl->log; }

Sord::World* World::rdf_world()  { return _impl->rdf_world; }
LilvWorld*   World::lilv_world() { return _impl->lilv_world; }

LV2Features& World::lv2_features() { return *_impl->lv2_features; }
Forge&       World::forge()        { return *_impl->forge; }
URIs&        World::uris()         { return *_impl->uris; }
URIMap&      World::uri_map()      { return *_impl->uri_map; }

bool
World::load_module(const char* name)
{
	Impl::Modules::iterator i = _impl->modules.find(name);
	if (i != _impl->modules.end()) {
		return true;
	}
	Glib::Module* lib = ingen_load_module(log(), name);
	Ingen::Module* (*module_load)() = NULL;
	if (lib && lib->get_symbol("ingen_module_load", (void*&)module_load)) {
		Module* module = module_load();
		if (module) {
			module->library = lib;
			module->load(this);
			_impl->modules.insert(make_pair(string(name), module));
			return true;
		}
	}

	log().error(fmt("Failed to load module `%1%'\n") % name);
	delete lib;
	return false;
}

bool
World::run_module(const char* name)
{
	Impl::Modules::iterator i = _impl->modules.find(name);
	if (i == _impl->modules.end()) {
		log().error(fmt("Attempt to run unloaded module `%1%'\n") % name);
		return false;
	}

	i->second->run(this);
	return true;
}

/** Get an interface for a remote engine at @a url
 */
SPtr<Interface>
World::new_interface(const Raul::URI& engine_uri,
                     SPtr<Interface>  respondee)
{
	const Impl::InterfaceFactories::const_iterator i = _impl->interface_factories.find(engine_uri.scheme());
	if (i == _impl->interface_factories.end()) {
		log().warn(fmt("Unknown URI scheme `%1%'\n") % engine_uri.scheme());
		return SPtr<Interface>();
	}

	return i->second(this, engine_uri, respondee);
}

/** Run a script of type @a mime_type at filename @a filename */
bool
World::run(const std::string& mime_type, const std::string& filename)
{
	const Impl::ScriptRunners::const_iterator i = _impl->script_runners.find(mime_type);
	if (i == _impl->script_runners.end()) {
		log().warn(fmt("Unknown script MIME type `%1%'\n") % mime_type);
		return false;
	}

	return i->second(this, filename.c_str());
}

void
World::add_interface_factory(const std::string& scheme, InterfaceFactory factory)
{
	_impl->interface_factories.insert(make_pair(scheme, factory));
}

void
World::set_jack_uuid(const std::string& uuid)
{
	_impl->jack_uuid = uuid;
}

std::string
World::jack_uuid()
{
	return _impl->jack_uuid;
}

} // namespace Ingen
