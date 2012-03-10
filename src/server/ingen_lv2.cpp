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

#include "ingen/ServerInterface.hpp"
#include "ingen/serialisation/Parser.hpp"
#include "ingen/shared/Configuration.hpp"
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

/** Record of a patch in this Ingen LV2 bundle */
struct LV2Patch {
	LV2Patch(const std::string& u, const std::string& f);

	const std::string uri;
	const std::string filename;
	LV2_Descriptor    descriptor;
};

class Lib {
public:
	Lib();
	~Lib();

	typedef std::vector< SharedPtr<const LV2Patch> > Patches;

	Patches                      patches;
	Ingen::Shared::Configuration conf;
	int                          argc;
	char**                       argv;
};

/** Library state (constructed/destructed on library load/unload) */
Lib lib;

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

/** LV2 plugin library entry point */
LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	return index < lib.patches.size() ? &lib.patches[index]->descriptor : NULL;
}

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
	Ingen::Shared::World* world;
	MainThread*           main;
};

static LV2_Handle
ingen_instantiate(const LV2_Descriptor*    descriptor,
                  double                   rate,
                  const char*              bundle_path,
                  const LV2_Feature*const* features)
{
	Shared::set_bundle_path(bundle_path);

	const LV2Patch* patch = NULL;
	for (Lib::Patches::iterator i = lib.patches.begin(); i != lib.patches.end(); ++i) {
		if (&(*i)->descriptor == descriptor) {
			patch = (*i).get();
			break;
		}
	}

	if (!patch) {
		Raul::error << "Could not find patch " << descriptor->URI << std::endl;
		return NULL;
	}

	IngenPlugin* plugin = (IngenPlugin*)malloc(sizeof(IngenPlugin));
	plugin->world = new Ingen::Shared::World(&lib.conf, lib.argc, lib.argv);
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

static const void*
ingen_extension_data(const char* uri)
{
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

/** Library constructor (called on shared library load) */
Lib::Lib()
	: argc(0)
	, argv(NULL)
{
	if (!Glib::thread_supported())
		Glib::thread_init();

	using namespace Ingen;

	Ingen::Shared::set_bundle_path_from_code((void*)&lv2_descriptor);

	Ingen::Shared::World* world = new Ingen::Shared::World(&conf, argc, argv);
	if (!world->load_module("serialisation")) {
		delete world;
		return;
	}

	assert(world->parser());

	typedef Serialisation::Parser::PatchRecords Records;

	const std::string manifest_path = Shared::bundle_file_path("manifest.ttl");
	const std::string manifest_uri  = Glib::filename_to_uri(manifest_path);
	const SerdNode    base_node     = serd_node_from_string(
		SERD_URI, (const uint8_t*)manifest_uri.c_str());

	SerdEnv* env = serd_env_new(&base_node);
	Records records(
		world->parser()->find_patches(world, env, manifest_uri));
	serd_env_free(env);

	for (Records::iterator i = records.begin(); i != records.end(); ++i) {
		uint8_t* path = serd_file_uri_parse((const uint8_t*)i->file_uri.c_str(), NULL);
		if (path) {
			patches.push_back(
				SharedPtr<const LV2Patch>(
					new LV2Patch(i->patch_uri.str(), (const char*)path)));
		}
		free(path);
	}

	delete world;
}

/** Library destructor (called on shared library unload) */
Lib::~Lib()
{
}

} // extern "C"
