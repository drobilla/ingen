/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include <boost/utility.hpp>

#include <glibmm/module.h>
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>

#include "lilv/lilv.h"
#include "raul/log.hpp"
#include "sord/sordmm.hpp"

#include "ingen/EngineBase.hpp"
#include "ingen/shared/Module.hpp"
#include "ingen/shared/World.hpp"
#include "ingen/shared/runtime_paths.hpp"
#include "ingen/shared/LV2Features.hpp"
#include "ingen/shared/LV2URIMap.hpp"

#define LOG(s) s << "[Module] "

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {

/** Load a dynamic module from the default path.
 *
 * This will check in the directories specified in the environment variable
 * INGEN_MODULE_PATH (typical colon delimited format), then the default module
 * installation directory (ie /usr/local/lib/ingen), in that order.
 *
 * \param name The base name of the module, e.g. "ingen_serialisation"
 */
static SharedPtr<Glib::Module>
ingen_load_module(const string& name)
{
	Glib::Module* module = NULL;

	// Search INGEN_MODULE_PATH first
	bool module_path_found;
	string module_path = Glib::getenv("INGEN_MODULE_PATH", module_path_found);
	if (module_path_found) {
		string dir;
		istringstream iss(module_path);
		while (getline(iss, dir, ':')) {
			string filename = Shared::module_path(name, dir);
			if (Glib::file_test(filename, Glib::FILE_TEST_EXISTS)) {
				module = new Glib::Module(filename, Glib::MODULE_BIND_LAZY);
				if (*module) {
					LOG(info) << "Loading " << filename << endl;
					return SharedPtr<Glib::Module>(module);
				} else {
					delete module;
					error << Glib::Module::get_last_error() << endl;
				}
			}
		}
	}

	// Try default directory if not found
	module = new Glib::Module(Shared::module_path(name), Glib::MODULE_BIND_LAZY);

	// FIXME: SEGV on exit without this
	module->make_resident();

	if (*module) {
		LOG(info) << "Loading " << Shared::module_path(name) << endl;
		return SharedPtr<Glib::Module>(module);
	} else if (!module_path_found) {
		LOG(error) << "Unable to find " << name
			<< " (" << Glib::Module::get_last_error() << ")" << endl;
		return SharedPtr<Glib::Module>();
	} else {
		LOG(error) << "Unable to load " << name << " from " << module_path
			<< " (" << Glib::Module::get_last_error() << ")" << endl;
		LOG(error) << "Is Ingen installed?" << endl;
		return SharedPtr<Glib::Module>();
	}
}

class World::Pimpl {
public:
	Pimpl(Raul::Configuration* conf, int& a_argc, char**& a_argv)
		: argc(a_argc)
		, argv(a_argv)
		, conf(conf)
		, lv2_features(NULL)
		, rdf_world(new Sord::World())
		, uris(new Shared::URIs())
		, lv2_uri_map(new Ingen::Shared::LV2URIMap(*uris.get()))
		, lilv_world(lilv_world_new())
	{
		lv2_features = new Ingen::Shared::LV2Features();
		lv2_features->add_feature(lv2_uri_map);
		lilv_world_load_all(lilv_world);

		// Set up RDF namespaces
		rdf_world->add_prefix("atom",    "http://lv2plug.in/ns/ext/atom#");
		rdf_world->add_prefix("ctx",     "http://lv2plug.in/ns/ext/contexts#");
		rdf_world->add_prefix("doap",    "http://usefulinc.com/ns/doap#");
		rdf_world->add_prefix("ingen",   "http://drobilla.net/ns/ingen#");
		rdf_world->add_prefix("lv2",     "http://lv2plug.in/ns/lv2core#");
		rdf_world->add_prefix("lv2ev",   "http://lv2plug.in/ns/ext/event#");
		rdf_world->add_prefix("lv2midi", "http://lv2plug.in/ns/ext/midi#");
		rdf_world->add_prefix("midi",    "http://drobilla.net/ns/ext/midi#");
		rdf_world->add_prefix("owl",     "http://www.w3.org/2002/07/owl#");
		rdf_world->add_prefix("rdfs",    "http://www.w3.org/2000/01/rdf-schema#");
		rdf_world->add_prefix("xsd",     "http://www.w3.org/2001/XMLSchema#");
	}

	virtual ~Pimpl()
	{
		serialiser.reset();
		parser.reset();

		local_engine.reset();

		modules.clear();
		interface_factories.clear();
		script_runners.clear();

		lilv_world_free(lilv_world);
		lilv_world = NULL;

		delete rdf_world;
		rdf_world = NULL;

		delete lv2_features;
		lv2_features = NULL;

		uris.reset();
	}

	typedef std::map< const std::string, SharedPtr<Module> > Modules;
	Modules modules;

	typedef std::map<const std::string, World::InterfaceFactory> InterfaceFactories;
	InterfaceFactories interface_factories;

	typedef bool (*ScriptRunner)(World* world, const char* filename);
	typedef std::map<const std::string, ScriptRunner> ScriptRunners;
	ScriptRunners script_runners;

	int&                                 argc;
	char**&                              argv;
	Raul::Configuration*                 conf;
	LV2Features*                         lv2_features;
	Sord::World*                         rdf_world;
	SharedPtr<URIs>                      uris;
	SharedPtr<LV2URIMap>                 lv2_uri_map;
	SharedPtr<ServerInterface>           engine;
	SharedPtr<EngineBase>                local_engine;
	SharedPtr<Serialisation::Serialiser> serialiser;
	SharedPtr<Serialisation::Parser>     parser;
	SharedPtr<Store>                     store;
	SharedPtr<ClientInterface>           client;
	LilvWorld*                           lilv_world;
	std::string                          jack_uuid;
};

World::World(Raul::Configuration* conf, int& argc, char**& argv)
	: _impl(new Pimpl(conf, argc, argv))
{
}

World::~World()
{
	unload_modules();
	delete _impl;
}

void World::set_local_engine(SharedPtr<EngineBase> e)              { _impl->local_engine = e; }
void World::set_engine(SharedPtr<ServerInterface> e)               { _impl->engine = e; }
void World::set_serialiser(SharedPtr<Serialisation::Serialiser> s) { _impl->serialiser = s; }
void World::set_parser(SharedPtr<Serialisation::Parser> p)         { _impl->parser = p; }
void World::set_store(SharedPtr<Store> s)                          { _impl->store = s; }
void World::set_client(SharedPtr<ClientInterface> c)               { _impl->client = c; }
void World::set_conf(Raul::Configuration* c)                       { _impl->conf = c; }

int&                                 World::argc()         { return _impl->argc; }
char**&                              World::argv()         { return _impl->argv; }
SharedPtr<EngineBase>                World::local_engine() { return _impl->local_engine; }
SharedPtr<ServerInterface>           World::engine()       { return _impl->engine; }
SharedPtr<Serialisation::Serialiser> World::serialiser()   { return _impl->serialiser; }
SharedPtr<Serialisation::Parser>     World::parser()       { return _impl->parser; }
SharedPtr<Store>                     World::store()        { return _impl->store; }
SharedPtr<ClientInterface>           World::client()       { return _impl->client; }
Raul::Configuration*                 World::conf()         { return _impl->conf; }
LV2Features*                         World::lv2_features() { return _impl->lv2_features; }

LilvWorld*           World::lilv_world()  { return _impl->lilv_world; }
Sord::World*         World::rdf_world()   { return _impl->rdf_world; }
SharedPtr<URIs>      World::uris()        { return _impl->uris; }
SharedPtr<LV2URIMap> World::lv2_uri_map() { return _impl->lv2_uri_map; }

/** Load an Ingen module.
 * @return true on success, false on failure
 */
bool
World::load_module(const char* name)
{
	Pimpl::Modules::iterator i = _impl->modules.find(name);
	if (i != _impl->modules.end()) {
		LOG(info) << "Module `" << name << "' already loaded" << endl;
		return true;
	}
	SharedPtr<Glib::Module> lib = ingen_load_module(name);
	Ingen::Shared::Module* (*module_load)() = NULL;
	if (lib && lib->get_symbol("ingen_module_load", (void*&)module_load)) {
		Module* module = module_load();
		module->library = lib;
		module->load(this);
		_impl->modules.insert(make_pair(string(name), module));
		return true;
	} else {
		LOG(error) << "Failed to load module `" << name << "'" << endl;
		return false;
	}
}

bool
World::run_module(const char* name)
{
	Pimpl::Modules::iterator i = _impl->modules.find(name);
	if (i == _impl->modules.end()) {
		LOG(error) << "Attempt to run unloaded module `" << name << "'" << endl;
		return false;
	}

	i->second->run(this);
	return true;
}
	
/** Unload all loaded Ingen modules.
 */
void
World::unload_modules()
{
	_impl->modules.clear();
}

/** Get an interface for a remote engine at @a url
 */
SharedPtr<ServerInterface>
World::interface(
	const std::string&         engine_url,
	SharedPtr<ClientInterface> respond_to)
{
	const string scheme = engine_url.substr(0, engine_url.find(":"));
	const Pimpl::InterfaceFactories::const_iterator i = _impl->interface_factories.find(scheme);
	if (i == _impl->interface_factories.end()) {
		warn << "Unknown URI scheme `" << scheme << "'" << endl;
		return SharedPtr<ServerInterface>();
	}

	return i->second(this, engine_url, respond_to);
}

/** Run a script of type @a mime_type at filename @a filename */
bool
World::run(const std::string& mime_type, const std::string& filename)
{
	const Pimpl::ScriptRunners::const_iterator i = _impl->script_runners.find(mime_type);
	if (i == _impl->script_runners.end()) {
		warn << "Unknown script MIME type `" << mime_type << "'" << endl;
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

} // namespace Shared
} // namespace Ingen
