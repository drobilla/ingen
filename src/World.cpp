/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "ingen/Configuration.hpp"
#include "ingen/DataAccess.hpp"
#include "ingen/EngineBase.hpp"
#include "ingen/Forge.hpp"
#include "ingen/InstanceAccess.hpp"
#include "ingen/LV2Features.hpp"
#include "ingen/Log.hpp"
#include "ingen/Module.hpp"
#include "ingen/Parser.hpp"
#include "ingen/Serialiser.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "ingen/filesystem.hpp"
#include "ingen/ingen.h"
#include "ingen/runtime_paths.hpp"
#include "lilv/lilv.h"
#include "sord/sordmm.hpp"

using std::string;

namespace ingen {

class EngineBase;
class Interface;
class Parser;
class Serialiser;
class Store;

/** Load a dynamic module from the default path.
 *
 * This will check in the directories specified in the environment variable
 * INGEN_MODULE_PATH (typical colon delimited format), then the default module
 * installation directory (ie /usr/local/lib/ingen), in that order.
 *
 * \param name The base name of the module, e.g. "ingen_jack"
 */
static std::unique_ptr<Library>
ingen_load_library(Log& log, const string& name)
{
	std::unique_ptr<Library> library;

	// Search INGEN_MODULE_PATH first
	const char* const module_path = getenv("INGEN_MODULE_PATH");
	if (module_path) {
		string dir;
		std::istringstream iss(module_path);
		while (getline(iss, dir, search_path_separator)) {
			FilePath filename = ingen::ingen_module_path(name, FilePath(dir));
			if (filesystem::exists(filename)) {
				library = std::unique_ptr<Library>(new Library(filename));
				if (*library) {
					return library;
				} else {
					log.error(Library::get_last_error());
				}
			}
		}
	}

	// Try default directory if not found
	library = std::unique_ptr<Library>(new Library(ingen::ingen_module_path(name)));

	if (*library) {
		return library;
	} else if (!module_path) {
		log.error(fmt("Unable to find %1% (%2%)\n")
		          % name % Library::get_last_error());
		return nullptr;
	} else {
		log.error(fmt("Unable to load %1% from %2% (%3%)\n")
		          % name % module_path % Library::get_last_error());
		return nullptr;
	}
}

class World::Impl {
public:
	Impl(LV2_URID_Map*   map,
	     LV2_URID_Unmap* unmap,
	     LV2_Log_Log*    lv2_log)
		: argc(nullptr)
		, argv(nullptr)
		, lv2_features(nullptr)
		, rdf_world(new Sord::World())
		, lilv_world(lilv_world_new(), lilv_world_free)
		, uri_map(log, map, unmap)
		, forge(uri_map)
		, uris(forge, &uri_map, lilv_world.get())
		, conf(forge)
		, log(lv2_log, uris)
	{
		lv2_features = new LV2Features();
		lv2_features->add_feature(uri_map.urid_map_feature());
		lv2_features->add_feature(uri_map.urid_unmap_feature());
		lv2_features->add_feature(SPtr<InstanceAccess>(new InstanceAccess()));
		lv2_features->add_feature(SPtr<DataAccess>(new DataAccess()));
		lv2_features->add_feature(SPtr<Log::Feature>(new Log::Feature()));
		lilv_world_load_all(lilv_world.get());

		// Set up RDF namespaces
		rdf_world->add_prefix("atom",  "http://lv2plug.in/ns/ext/atom#");
		rdf_world->add_prefix("doap",  "http://usefulinc.com/ns/doap#");
		rdf_world->add_prefix("ingen", INGEN_NS);
		rdf_world->add_prefix("lv2",   "http://lv2plug.in/ns/lv2core#");
		rdf_world->add_prefix("midi",  "http://lv2plug.in/ns/ext/midi#");
		rdf_world->add_prefix("owl",   "http://www.w3.org/2002/07/owl#");
		rdf_world->add_prefix("patch", "http://lv2plug.in/ns/ext/patch#");
		rdf_world->add_prefix("rdf",   "http://www.w3.org/1999/02/22-rdf-syntax-ns#");
		rdf_world->add_prefix("rdfs",  "http://www.w3.org/2000/01/rdf-schema#");
		rdf_world->add_prefix("xsd",   "http://www.w3.org/2001/XMLSchema#");

		// Load internal 'plugin' information into lilv world
		LilvNode* rdf_type = lilv_new_uri(
			lilv_world.get(), "http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
		LilvNode* ingen_Plugin = lilv_new_uri(
			lilv_world.get(), INGEN__Plugin);
		LilvNodes* internals = lilv_world_find_nodes(
			lilv_world.get(), nullptr, rdf_type, ingen_Plugin);
		LILV_FOREACH(nodes, i, internals) {
			const LilvNode* internal = lilv_nodes_get(internals, i);
			lilv_world_load_resource(lilv_world.get(), internal);
		}
		lilv_nodes_free(internals);
		lilv_node_free(rdf_type);
		lilv_node_free(ingen_Plugin);
	}

	~Impl()
	{
		if (engine) {
			engine->quit();
		}

		// Delete module objects but save pointers to libraries
		typedef std::list<std::unique_ptr<Library>> Libs;
		Libs libs;
		for (auto& m : modules) {
			libs.emplace_back(std::move(m.second->library));
			delete m.second;
		}

		serialiser.reset();
		parser.reset();
		interface.reset();
		engine.reset();
		store.reset();

		interface_factories.clear();
		script_runners.clear();

		delete lv2_features;

		// Module libraries go out of scope and close here
	}

	typedef std::map<std::string, Module*> Modules;
	Modules modules;

	typedef std::map<const std::string, World::InterfaceFactory> InterfaceFactories;
	InterfaceFactories interface_factories;

	typedef bool (*ScriptRunner)(World& world, const char* filename);
	typedef std::map<const std::string, ScriptRunner> ScriptRunners;
	ScriptRunners script_runners;

	using LilvWorldUPtr = std::unique_ptr<LilvWorld, decltype(&lilv_world_free)>;

	int*              argc;
	char***           argv;
	LV2Features*      lv2_features;
	UPtr<Sord::World> rdf_world;
	LilvWorldUPtr     lilv_world;
	URIMap            uri_map;
	Forge             forge;
	URIs              uris;
	LV2_Log_Log*      lv2_log;
	Configuration     conf;
	Log               log;
	SPtr<Interface>   interface;
	SPtr<EngineBase>  engine;
	SPtr<Serialiser>  serialiser;
	SPtr<Parser>      parser;
	SPtr<Store>       store;
	std::mutex        rdf_mutex;
	std::string       jack_uuid;
};

World::World(LV2_URID_Map* map, LV2_URID_Unmap* unmap, LV2_Log_Log* log)
	: _impl(new Impl(map, unmap, log))
{
	_impl->serialiser = SPtr<Serialiser>(new Serialiser(*this));
	_impl->parser     = SPtr<Parser>(new Parser());
}

World::~World()
{
	delete _impl;
}

void
World::load_configuration(int& argc, char**& argv)
{
	_impl->argc = &argc;
	_impl->argv = &argv;

	// Parse default configuration files
	const auto files = _impl->conf.load_default("ingen", "options.ttl");
	for (const auto& f : files) {
		_impl->log.info(fmt("Loaded configuration %1%\n") % f);
	}

	// Parse command line options, overriding configuration file values
	_impl->conf.parse(argc, argv);
	_impl->log.set_flush(_impl->conf.option("flush-log").get<int32_t>());
	_impl->log.set_trace(_impl->conf.option("trace").get<int32_t>());
}

void World::set_engine(SPtr<EngineBase> e)   { _impl->engine     = e; }
void World::set_interface(SPtr<Interface> i) { _impl->interface  = i; }
void World::set_store(SPtr<Store> s)         { _impl->store      = s; }

SPtr<EngineBase> World::engine()     { return _impl->engine; }
SPtr<Interface>  World::interface()  { return _impl->interface; }
SPtr<Parser>     World::parser()     { return _impl->parser; }
SPtr<Serialiser> World::serialiser() { return _impl->serialiser; }
SPtr<Store>      World::store()      { return _impl->store; }

int&           World::argc() { return *_impl->argc; }
char**&        World::argv() { return *_impl->argv; }
Configuration& World::conf() { return _impl->conf; }
Log&           World::log()  { return _impl->log; }

std::mutex& World::rdf_mutex() { return _impl->rdf_mutex; }

Sord::World* World::rdf_world()  { return _impl->rdf_world.get(); }
LilvWorld*   World::lilv_world() { return _impl->lilv_world.get(); }

LV2Features& World::lv2_features() { return *_impl->lv2_features; }
Forge&       World::forge()        { return _impl->forge; }
URIs&        World::uris()         { return _impl->uris; }
URIMap&      World::uri_map()      { return _impl->uri_map; }

bool
World::load_module(const char* name)
{
	auto i = _impl->modules.find(name);
	if (i != _impl->modules.end()) {
		return true;
	}
	log().info(fmt("Loading %1% module\n") % name);
	std::unique_ptr<ingen::Library> lib = ingen_load_library(log(), name);
	ingen::Module* (*module_load)() =
		lib ? (ingen::Module* (*)())lib->get_function("ingen_module_load")
		    : nullptr;
	if (module_load) {
		Module* module = module_load();
		if (module) {
			module->library = std::move(lib);
			module->load(*this);
			_impl->modules.emplace(string(name), module);
			return true;
		}
	}

	log().error(fmt("Failed to load module `%1%' (%2%)\n") % name % lib->get_last_error());
	return false;
}

bool
World::run_module(const char* name)
{
	auto i = _impl->modules.find(name);
	if (i == _impl->modules.end()) {
		log().error(fmt("Attempt to run unloaded module `%1%'\n") % name);
		return false;
	}

	i->second->run(*this);
	return true;
}

/** Get an interface for a remote engine at `engine_uri`
 */
SPtr<Interface>
World::new_interface(const URI& engine_uri, SPtr<Interface> respondee)
{
	const Impl::InterfaceFactories::const_iterator i =
	        _impl->interface_factories.find(std::string(engine_uri.scheme()));
	if (i == _impl->interface_factories.end()) {
		log().warn(fmt("Unknown URI scheme `%1%'\n") % engine_uri.scheme());
		return SPtr<Interface>();
	}

	return i->second(*this, engine_uri, respondee);
}

/** Run a script of type `mime_type` at filename `filename` */
bool
World::run(const std::string& mime_type, const std::string& filename)
{
	const Impl::ScriptRunners::const_iterator i = _impl->script_runners.find(mime_type);
	if (i == _impl->script_runners.end()) {
		log().warn(fmt("Unknown script MIME type `%1%'\n") % mime_type);
		return false;
	}

	return i->second(*this, filename.c_str());
}

void
World::add_interface_factory(const std::string& scheme, InterfaceFactory factory)
{
	_impl->interface_factories.emplace(scheme, factory);
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

} // namespace ingen
