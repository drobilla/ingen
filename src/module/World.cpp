/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#include "ingen-config.h"
#include <boost/utility.hpp>
#include <glibmm/module.h>
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#ifdef HAVE_SLV2
#include "slv2/slv2.h"
#endif
#include "raul/log.hpp"
#include "redlandmm/World.hpp"
#include "shared/runtime_paths.hpp"
#include "shared/LV2Features.hpp"
#include "shared/LV2URIMap.hpp"
#include "World.hpp"

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
load_module(const string& name)
{
	Glib::Module* module = NULL;

	// Search INGEN_MODULE_PATH first
	bool module_path_found;
	string module_path = Glib::getenv("INGEN_MODULE_PATH", module_path_found);
	if (module_path_found) {
		string dir;
		istringstream iss(module_path);
		while (getline(iss, dir, ':')) {
			string filename = Glib::Module::build_path(dir, name);
			if (Glib::file_test(filename, Glib::FILE_TEST_EXISTS)) {
				module = new Glib::Module(filename, Glib::MODULE_BIND_LAZY);
				if (*module) {
					LOG(info) << "Loaded `" <<  name << "' from " << filename << endl;
					return SharedPtr<Glib::Module>(module);
				} else {
					delete module;
					error << Glib::Module::get_last_error() << endl;
				}
			}
		}
	}

	// Try default directory if not found
	module = new Glib::Module(
			Shared::module_path(name),
            Glib::MODULE_BIND_LAZY);

	// FIXME: SEGV on exit without this
	module->make_resident();

	if (*module) {
		LOG(info) << "Loaded `" <<  name << "' from " << INGEN_MODULE_DIR << endl;
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


struct WorldImpl : public boost::noncopyable {
	WorldImpl(Raul::Configuration* conf, int& a_argc, char**& a_argv)
		: argc(a_argc)
		, argv(a_argv)
		, conf(conf)
		, lv2_features(NULL)
		, rdf_world(new Redland::World())
		, uris(new Shared::LV2URIMap())
#ifdef HAVE_SLV2
		, slv2_world(slv2_world_new_using_rdf_world(rdf_world->c_obj()))
#endif
	{
#ifdef HAVE_SLV2
		lv2_features = new Ingen::Shared::LV2Features();
		lv2_features->add_feature(LV2_URI_MAP_URI, uris);
		slv2_world_load_all(slv2_world);
#endif

		// Set up RDF namespaces
		rdf_world->add_prefix("dc",      "http://purl.org/dc/elements/1.1/");
		rdf_world->add_prefix("doap",    "http://usefulinc.com/ns/doap#");
		rdf_world->add_prefix("ingen",   "http://drobilla.net/ns/ingen#");
		rdf_world->add_prefix("ingenui", "http://drobilla.net/ns/ingenuity#");
		rdf_world->add_prefix("lv2",     "http://lv2plug.in/ns/lv2core#");
		rdf_world->add_prefix("lv2ev",   "http://lv2plug.in/ns/ext/event#");
		rdf_world->add_prefix("ctx",     "http://lv2plug.in/ns/dev/contexts#");
		rdf_world->add_prefix("lv2midi", "http://lv2plug.in/ns/ext/midi#");
		rdf_world->add_prefix("midi",    "http://drobilla.net/ns/dev/midi#");
		rdf_world->add_prefix("owl",     "http://www.w3.org/2002/07/owl#");
		rdf_world->add_prefix("rdfs",    "http://www.w3.org/2000/01/rdf-schema#");
		rdf_world->add_prefix("sp",      "http://lv2plug.in/ns/dev/string-port#");
		rdf_world->add_prefix("xsd",     "http://www.w3.org/2001/XMLSchema#");
	}

	virtual ~WorldImpl()
	{
		local_engine.reset();

		modules.clear();
		interface_factories.clear();
		script_runners.clear();

	#ifdef HAVE_SLV2
		slv2_world_free(slv2_world);
		slv2_world = NULL;
	#endif

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
	Redland::World*                      rdf_world;
	SharedPtr<LV2URIMap>                 uris;
    SharedPtr<EngineInterface>           engine;
	SharedPtr<Engine>                    local_engine;
    SharedPtr<Serialisation::Serialiser> serialiser;
    SharedPtr<Serialisation::Parser>     parser;
    SharedPtr<Store>                     store;
#ifdef HAVE_SLV2
    SLV2World                            slv2_world;
#endif
};


World::World(Raul::Configuration* conf, int& argc, char**& argv)
	: _impl(new WorldImpl(conf, argc, argv))
{
}


World::~World()
{
	unload_all();
	delete _impl;
}

void World::set_local_engine(SharedPtr<Engine> e)                  { _impl->local_engine = e; }
void World::set_engine(SharedPtr<EngineInterface> e)               { _impl->engine = e; }
void World::set_serialiser(SharedPtr<Serialisation::Serialiser> s) { _impl->serialiser = s; }
void World::set_parser(SharedPtr<Serialisation::Parser> p)         { _impl->parser = p; }
void World::set_store(SharedPtr<Store> s)                          { _impl->store = s; }
void World::set_conf(Raul::Configuration* c)                       { _impl->conf = c; }

int&                                 World::argc()         { return _impl->argc; }
char**&                              World::argv()         { return _impl->argv; }
SharedPtr<Engine>                    World::local_engine() { return _impl->local_engine; }
SharedPtr<EngineInterface>           World::engine()       { return _impl->engine; }
SharedPtr<Serialisation::Serialiser> World::serialiser()   { return _impl->serialiser; }
SharedPtr<Serialisation::Parser>     World::parser()       { return _impl->parser; }
SharedPtr<Store>                     World::store()        { return _impl->store; }
Raul::Configuration*                 World::conf()         { return _impl->conf; }
LV2Features*                         World::lv2_features() { return _impl->lv2_features; }

#ifdef HAVE_SLV2
SLV2World            World::slv2_world() { return _impl->slv2_world; }
#endif
Redland::World*      World::rdf_world() { return _impl->rdf_world; }
SharedPtr<LV2URIMap> World::uris()      { return _impl->uris; }


/** Load an Ingen module.
 * @return true on success, false on failure
 */
bool
World::load(const char* name)
{
	SharedPtr<Glib::Module> lib = load_module(name);
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


/** Unload all loaded Ingen modules.
 */
void
World::unload_all()
{
	_impl->modules.clear();
}


/** Get an interface for a remote engine at @a url
 */
SharedPtr<Ingen::Shared::EngineInterface>
World::interface(const std::string& url)
{
	const string scheme = url.substr(0, url.find(":"));
	const WorldImpl::InterfaceFactories::const_iterator i = _impl->interface_factories.find(scheme);
	if (i == _impl->interface_factories.end()) {
		warn << "Unknown URI scheme `" << scheme << "'" << endl;
		return SharedPtr<Ingen::Shared::EngineInterface>();
	}

	return i->second(this, url);
}


/** Run a script of type @a mime_type at filename @a filename */
bool
World::run(const std::string& mime_type, const std::string& filename)
{
	const WorldImpl::ScriptRunners::const_iterator i = _impl->script_runners.find(mime_type);
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



} // namespace Shared
} // namespace Ingen
