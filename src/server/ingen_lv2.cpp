/* Ingen.LV2 - A thin wrapper which allows Ingen to run as an LV2 plugin.
 * Copyright 2008-2011 David Robillard <http://drobilla.net>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>

#include <string>
#include <vector>

#include <glib.h>
#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <glibmm/timer.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

#include "ingen/ServerInterface.hpp"
#include "ingen/serialisation/Parser.hpp"
#include "ingen/serialisation/Serialiser.hpp"
#include "ingen/shared/Configuration.hpp"
#include "ingen/shared/Store.hpp"
#include "ingen/shared/World.hpp"
#include "ingen/shared/runtime_paths.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/Thread.hpp"
#include "raul/log.hpp"

#include "AudioBuffer.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "PatchImpl.hpp"
#include "PostProcessor.hpp"
#include "ProcessContext.hpp"
#include "ServerInterfaceImpl.hpp"
#include "ThreadManager.hpp"

#define NS_INGEN "http://drobilla.net/ns/ingen#"
#define NS_RDF   "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS  "http://www.w3.org/2000/01/rdf-schema#"

/** Record of a patch in this Ingen LV2 bundle */
struct LV2Patch {
	LV2Patch(const std::string& u, const std::string& f);

	const std::string uri;
	const std::string filename;
	LV2_Descriptor    descriptor;
};

class Lib {
public:
	Lib(const char* bundle_path);

	typedef std::vector< SharedPtr<const LV2Patch> > Patches;

	Patches patches;
};

namespace Ingen {
namespace Server {

class LV2Driver;

class LV2Port : public DriverPort
{
public:
	LV2Port(LV2Driver* driver, DuplexPort* patch_port)
		: DriverPort(patch_port)
		, _driver(driver)
		, _buffer(NULL)
	{}

	// TODO: LV2 dynamic ports
	void create() {}
	void destroy() {}
	void move(const Raul::Path& path) {}

	void pre_process(ProcessContext& context) {
		if (!is_input() || !_buffer)
			return;

		if (_patch_port->is_a(PortType::AUDIO)) {
			AudioBuffer* patch_buf = (AudioBuffer*)_patch_port->buffer(0).get();
			patch_buf->copy((Sample*)_buffer, 0, context.nframes() - 1);
		} else if (_patch_port->is_a(PortType::EVENTS)) {
			//Raul::warn << "TODO: LV2 event I/O" << std::endl;
		}
	}

	void post_process(ProcessContext& context) {
		if (is_input() || !_buffer)
			return;

		if (_patch_port->is_a(PortType::AUDIO)) {
			AudioBuffer* patch_buf = (AudioBuffer*)_patch_port->buffer(0).get();
			memcpy((Sample*)_buffer, patch_buf->data(), context.nframes() * sizeof(Sample));
		} else if (_patch_port->is_a(PortType::EVENTS)) {
			//Raul::warn << "TODO: LV2 event I/O" << std::endl;
		}
	}

	void* buffer() const        { return _buffer; }
	void  set_buffer(void* buf) { _buffer = buf; }

private:
	LV2Driver* _driver;
	void*      _buffer;
};

class LV2Driver : public Ingen::Server::Driver {
private:
	typedef std::vector<LV2Port*> Ports;

public:
	LV2Driver(Engine& engine, SampleCount buffer_size, SampleCount sample_rate)
		: _context(engine)
		, _root_patch(NULL)
		, _buffer_size(buffer_size)
		, _sample_rate(sample_rate)
		, _frame_time(0)
	{}

	void run(uint32_t nframes) {
		_context.locate(_frame_time, nframes, 0);

		for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i)
			(*i)->pre_process(_context);

		_context.engine().process_events(_context);

		if (_root_patch)
			_root_patch->process(_context);

		for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i)
			(*i)->post_process(_context);

		_frame_time += nframes;
	}

	virtual void       set_root_patch(PatchImpl* patch) { _root_patch = patch; }
	virtual PatchImpl* root_patch()                     { return _root_patch; }

	virtual void add_port(DriverPort* port) {
		// Note this doesn't have to be realtime safe since there's no dynamic LV2 ports
		ThreadManager::assert_thread(THREAD_PROCESS);
		assert(dynamic_cast<LV2Port*>(port));
		assert(port->patch_port()->index() == _ports.size());
		_ports.push_back((LV2Port*)port);
	}

	virtual Raul::Deletable* remove_port(const Raul::Path& path,
	                                     DriverPort** port=NULL) {
		// Note this doesn't have to be realtime safe since there's no dynamic LV2 ports
		ThreadManager::assert_thread(THREAD_PROCESS);

		for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i) {
			if ((*i)->patch_port()->path() == path) {
				_ports.erase(i);
				return NULL;
			}
		}

		Raul::warn << "Unable to find port " << path << std::endl;
		return NULL;
	}

	virtual DriverPort* create_port(DuplexPort* patch_port) {
		return new LV2Port(this, patch_port);
	}

	virtual DriverPort* driver_port(const Raul::Path& path) {
		ThreadManager::assert_thread(THREAD_PROCESS);

		for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i)
			if ((*i)->patch_port()->path() == path)
				return (*i);

		return NULL;
	}

	virtual SampleCount block_length() const { return _buffer_size; }
	virtual SampleCount sample_rate()  const { return _sample_rate; }
	virtual SampleCount frame_time()   const { return _frame_time;}

	virtual bool            is_realtime() const { return true; }
	virtual ProcessContext& context()           { return _context; }

	Ports& ports() { return _ports; }

private:
	ProcessContext _context;
	PatchImpl*     _root_patch;
	SampleCount    _buffer_size;
	SampleCount    _sample_rate;
	SampleCount    _frame_time;
	Ports          _ports;
};

} // namespace Server
} // namespace Ingen

extern "C" {

using namespace Ingen;
using namespace Ingen::Server;

class MainThread : public Raul::Thread
{
public:
	MainThread(SharedPtr<Engine> engine) : _engine(engine) {}

private:
	virtual void _run() {
		while (_engine->main_iteration()) {
			Glib::usleep(125000);  // 1/8 second
		}
	}

	SharedPtr<Engine> _engine;
};

struct IngenPlugin {
	Raul::Forge                   forge;
	Ingen::Shared::Configuration* conf;
	Ingen::Shared::World*         world;
	MainThread*                   main;
	LV2_URID_Map*                 map;
	int                           argc;
	char**                        argv;
};

static Lib::Patches
find_patches(const Glib::ustring& manifest_uri)
{
	Sord::World      world;
	const Sord::URI  base(world, manifest_uri);
	const Sord::Node nil;
	const Sord::URI  ingen_Patch (world, NS_INGEN "Patch");
	const Sord::URI  rdf_type    (world, NS_RDF   "type");
	const Sord::URI  rdfs_seeAlso(world, NS_RDFS  "seeAlso");

	SerdEnv*    env = serd_env_new(sord_node_to_serd_node(base.c_obj()));
	Sord::Model model(world, manifest_uri);
	model.load_file(env, SERD_TURTLE, manifest_uri);

	Lib::Patches patches;
	for (Sord::Iter i = model.find(nil, rdf_type, ingen_Patch); !i.end(); ++i) {
		const Sord::Node  patch     = i.get_subject();
		Sord::Iter        f         = model.find(patch, rdfs_seeAlso, nil);
		const std::string patch_uri = patch.to_c_string();
		if (!f.end()) {
			const uint8_t* file_uri  = f.get_object().to_u_string();
			uint8_t*       file_path = serd_file_uri_parse(file_uri, NULL);
			patches.push_back(boost::shared_ptr<const LV2Patch>(
				                  new LV2Patch(patch_uri, (const char*)file_path)));
			free(file_path);
		} else {
			Raul::error << "[Ingen] Patch has no rdfs:seeAlso" << endl;
		}
	}

	serd_env_free(env);
	return patches;
}

static LV2_Handle
ingen_instantiate(const LV2_Descriptor*    descriptor,
                  double                   rate,
                  const char*              bundle_path,
                  const LV2_Feature*const* features)
{
	Shared::set_bundle_path(bundle_path);
	Lib::Patches patches = find_patches(
		Glib::filename_to_uri(
			Shared::bundle_file_path("manifest.ttl")));

	const LV2Patch* patch = NULL;
	for (Lib::Patches::iterator i = patches.begin(); i != patches.end(); ++i) {
		if ((*i)->uri == descriptor->URI) {
			patch = (*i).get();
			break;
		}
	}

	if (!patch) {
		Raul::error << "Could not find patch " << descriptor->URI << std::endl;
		return NULL;
	}

	IngenPlugin* plugin = (IngenPlugin*)malloc(sizeof(IngenPlugin));
	plugin->conf          = new Ingen::Shared::Configuration(&plugin->forge);
	plugin->main          = NULL;
	plugin->map           = NULL;
	LV2_URID_Unmap* unmap = NULL;
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID_URI "#map")) {
			plugin->map = (LV2_URID_Map*)features[i]->data;
		} else if (!strcmp(features[i]->URI, LV2_URID_URI "#unmap")) {
			unmap = (LV2_URID_Unmap*)features[i]->data;
		}
	}

	plugin->world = new Ingen::Shared::World(
		plugin->conf, plugin->argc, plugin->argv, plugin->map, unmap);
	if (!plugin->world->load_module("serialisation")) {
		delete plugin->world;
		return NULL;
	}

	SharedPtr<Server::Engine> engine(new Server::Engine(plugin->world));
	plugin->world->set_local_engine(engine);
	plugin->main = new MainThread(engine);
	plugin->main->set_name("Main");

	SharedPtr<Server::ServerInterfaceImpl> interface(
		new Server::ServerInterfaceImpl(*engine.get()));

	plugin->world->set_engine(interface);
	engine->add_event_source(interface);

	Raul::Thread::get().set_context(Server::THREAD_PRE_PROCESS);
	Server::ThreadManager::single_threaded = true;

	// FIXME: fixed (or at least maximum) buffer size
	LV2Driver* driver = new LV2Driver(*engine.get(), rate, 4096);
	engine->set_driver(SharedPtr<Ingen::Server::Driver>(driver));

	engine->activate();
	Server::ThreadManager::single_threaded = true;

	ProcessContext context(*engine.get());
	context.locate(0, UINT_MAX, 0);

	engine->post_processor()->set_end_time(UINT_MAX);

	// TODO: Load only necessary plugins
	plugin->world->engine()->get("ingen:plugins");
	interface->process(*engine->post_processor(), context, false);
	engine->post_processor()->process();

	plugin->world->parser()->parse_file(plugin->world,
	                                    plugin->world->engine().get(),
	                                    patch->filename);

	while (!interface->empty()) {
		interface->process(*engine->post_processor(), context, false);
		engine->post_processor()->process();
	}

	engine->deactivate();

	plugin->world->load_module("osc_server");

	return (LV2_Handle)plugin;
}

static void
ingen_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	using namespace Ingen::Server;

	IngenPlugin*    me     = (IngenPlugin*)instance;
	Server::Engine* engine = (Server::Engine*)me->world->local_engine().get();
	LV2Driver*      driver = (LV2Driver*)engine->driver();
	if (port < driver->ports().size()) {
		driver->ports().at(port)->set_buffer(data);
		assert(driver->ports().at(port)->patch_port()->index() == port);
	} else {
		Raul::warn << "Connect to non-existent port " << port << std::endl;
	}
}

static void
ingen_activate(LV2_Handle instance)
{
	IngenPlugin* me = (IngenPlugin*)instance;
	me->world->local_engine()->activate();
	me->main->start();
}

static void
ingen_run(LV2_Handle instance, uint32_t sample_count)
{
	IngenPlugin*    me     = (IngenPlugin*)instance;
	Server::Engine* engine = (Server::Engine*)me->world->local_engine().get();
	// FIXME: don't do this every call
	Raul::Thread::get().set_context(Ingen::Server::THREAD_PROCESS);
	((LV2Driver*)engine->driver())->run(sample_count);
}

static void
ingen_deactivate(LV2_Handle instance)
{
	IngenPlugin* me = (IngenPlugin*)instance;
	me->world->local_engine()->deactivate();
}

static void
ingen_cleanup(LV2_Handle instance)
{
	IngenPlugin* me = (IngenPlugin*)instance;
	me->world->set_local_engine(SharedPtr<Ingen::Server::Engine>());
	me->world->set_engine(SharedPtr<Ingen::ServerInterface>());
	delete me->world;
	free(instance);
}

static void
get_state_features(const LV2_Feature* const* features,
                   LV2_State_Map_Path**      map,
                   LV2_State_Make_Path**     make)
{
	for (int i = 0; features[i]; ++i) {
		if (map && !strcmp(features[i]->URI, LV2_STATE__mapPath)) {
			*map = (LV2_State_Map_Path*)features[i]->data;
		} else if (make && !strcmp(features[i]->URI, LV2_STATE__makePath)) {
			*make = (LV2_State_Make_Path*)features[i]->data;
		}
	}
}

static void
ingen_save(LV2_Handle                instance,
           LV2_State_Store_Function  store,
           LV2_State_Handle          handle,
           uint32_t                  flags,
           const LV2_Feature* const* features)
{
	IngenPlugin* plugin = (IngenPlugin*)instance;

	LV2_State_Map_Path*  map_path  = NULL;
	LV2_State_Make_Path* make_path = NULL;
	get_state_features(features, &map_path, &make_path);
	if (!map_path || !make_path || !plugin->map) {
		Raul::error << "Missing state:mapPath, state:makePath, or urid:Map."
		            << endl;
		return;
	}

	LV2_URID ingen_file = plugin->map->map(plugin->map->handle, NS_INGEN "file");
	LV2_URID atom_Path = plugin->map->map(plugin->map->handle,
	                                      "http://lv2plug.in/ns/ext/atom#Path");

	char* real_path  = make_path->path(make_path->handle, "patch.ttl");
	char* state_path = map_path->abstract_path(map_path->handle, real_path);

	Ingen::Shared::Store::iterator root = plugin->world->store()->find("/");
	plugin->world->serialiser()->to_file(root->second, real_path);

	store(handle,
	      ingen_file,
	      state_path,
	      strlen(state_path) + 1,
	      atom_Path,
	      LV2_STATE_IS_POD);

	free(state_path);
	free(real_path);
}

static void
ingen_restore(LV2_Handle                  instance,
              LV2_State_Retrieve_Function retrieve,
              LV2_State_Handle            handle,
              uint32_t                    flags,
              const LV2_Feature* const*   features)
{
	IngenPlugin* plugin = (IngenPlugin*)instance;

	LV2_State_Map_Path* map_path = NULL;
	get_state_features(features, &map_path, NULL);
	if (!map_path) {
		Raul::error << "Missing state:mapPath" << endl;
		return;
	}

	LV2_URID ingen_file = plugin->map->map(plugin->map->handle, NS_INGEN "file");
	size_t   size;
	uint32_t type;
	uint32_t valflags;

	const void* path = retrieve(handle,
	                            ingen_file,
	                            &size, &type, &valflags);

	if (!path) {
		Raul::error << "Failed to restore ingen:file" << endl;
		return;
	}

	const char* state_path = (const char*)path;
	char* real_path = map_path->absolute_path(map_path->handle, state_path);

	plugin->world->parser()->parse_file(plugin->world,
	                                    plugin->world->engine().get(),
	                                    real_path);
	free(real_path);
}

const void*
ingen_extension_data(const char* uri)
{
	static const LV2_State_Interface state = { ingen_save, ingen_restore };
	if (!strcmp(uri, LV2_STATE__Interface)) {
		return &state;
	}
	return NULL;
}

LV2Patch::LV2Patch(const std::string& u, const std::string& f)
	: uri(u)
	, filename(f)
{
	descriptor.URI            = uri.c_str();
	descriptor.instantiate    = ingen_instantiate;
	descriptor.connect_port   = ingen_connect_port;
	descriptor.activate       = ingen_activate;
	descriptor.run            = ingen_run;
	descriptor.deactivate     = ingen_deactivate;
	descriptor.cleanup        = ingen_cleanup;
	descriptor.extension_data = ingen_extension_data;
}

Lib::Lib(const char* bundle_path)
{
	Ingen::Shared::set_bundle_path(bundle_path);
	patches = find_patches(
		Glib::filename_to_uri(
			Ingen::Shared::bundle_file_path("manifest.ttl")));
}

static void
lib_cleanup(LV2_Lib_Handle handle)
{
	Lib* lib = (Lib*)handle;
	delete lib;
}
	
static const LV2_Descriptor*
lib_get_plugin(LV2_Lib_Handle handle, uint32_t index)
{
	Lib* lib = (Lib*)handle;
	return index < lib->patches.size() ? &lib->patches[index]->descriptor : NULL;
}

/** LV2 plugin library entry point */
LV2_SYMBOL_EXPORT
const LV2_Lib_Descriptor*
lv2_lib_descriptor(const char*              bundle_path,
                   const LV2_Feature*const* features)
{
	Lib* lib = new Lib(bundle_path);
	static LV2_Lib_Descriptor desc = {
		lib, sizeof(LV2_Lib_Descriptor), lib_cleanup, lib_get_plugin
	};
	return &desc;
}

} // extern "C"
