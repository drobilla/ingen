/* Ingen.LV2 - A thin wrapper which allows Ingen to run as an LV2 plugin.
 * Copyright (C) 2008-2009 David Robillard <http://drobilla.net>
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
#include <glibmm/miscutils.h>
#include <glibmm/convert.h>
#include <glib.h>
#include "lv2.h"
#include "ingen-config.h"
#include "raul/log.hpp"
#include "raul/Thread.hpp"
#include "raul/SharedPtr.hpp"
#include "interface/EngineInterface.hpp"
#include "module/World.hpp"
#include "module/ingen_module.hpp"
#include "shared/runtime_paths.hpp"
#include "shared/Configuration.hpp"
#include "engine/AudioBuffer.hpp"
#include "engine/Driver.hpp"
#include "engine/Engine.hpp"
#include "engine/PatchImpl.hpp"
#include "engine/PostProcessor.hpp"
#include "engine/ProcessContext.hpp"
#include "engine/QueuedEngineInterface.hpp"
#include "engine/ThreadManager.hpp"
#include "serialisation/Parser.hpp"

using namespace Ingen;

namespace Ingen {
namespace LV2 {

/** Record of a patch found in this LV2 bundle */
struct LV2Patch {
	LV2Patch(const std::string& u, const std::string& f);

	const std::string uri;
	const std::string filename;
	LV2_Descriptor    descriptor;
};


/* Static library data */
typedef std::vector< SharedPtr<const LV2Patch> > LV2Patches;
static bool           initialised = false;
static LV2Patches     patches;
static Configuration  conf;
static int            argc = 0;
static char**         argv = NULL;
static Shared::World* world = NULL;


struct LV2Driver;

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

		if (_patch_port->buffer_type() == PortType::AUDIO) {
			AudioBuffer* patch_buf = (AudioBuffer*)_patch_port->buffer(0).get();
			patch_buf->copy((Sample*)_buffer, 0, context.nframes() - 1);
		} else if (_patch_port->buffer_type() == PortType::EVENTS) {
			//Raul::warn << "TODO: LV2 event I/O" << std::endl;
		}
	}

	void post_process(ProcessContext& context) {
		if (is_input() || !_buffer)
			return;

		if (_patch_port->buffer_type() == PortType::AUDIO) {
			AudioBuffer* patch_buf = (AudioBuffer*)_patch_port->buffer(0).get();
			memcpy((Sample*)_buffer, patch_buf->data(), context.nframes() * sizeof(Sample));
		} else if (_patch_port->buffer_type() == PortType::EVENTS) {
			//Raul::warn << "TODO: LV2 event I/O" << std::endl;
		}
	}

	void* buffer() const        { return _buffer; }
	void  set_buffer(void* buf) { _buffer = buf; }

private:
	LV2Driver* _driver;
	void*      _buffer;
};


struct LV2Driver : public Driver {
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

	virtual Raul::Deletable* remove_port(const Raul::Path& path, Ingen::DriverPort** port=NULL) {
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


	virtual bool supports(Shared::PortType port_type, Shared::EventType event_type) {
		return true;
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

} // namespace LV2
} // namespace Ingen


/* LV2 Plugin Interface */

extern "C" {


struct IngenPlugin {
	Ingen::Shared::World* world;
};


static LV2_Handle
ingen_instantiate(const LV2_Descriptor*    descriptor,
                  double                   rate,
                  const char*              bundle_path,
                  const LV2_Feature*const* features)
{
	Shared::set_bundle_path(bundle_path);

	using namespace LV2;

	const LV2Patch* patch = NULL;
	for (LV2Patches::iterator i = patches.begin(); i != patches.end(); ++i) {
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
	//plugin->world = ingen_world_new(&conf, argc, argv);
	plugin->world = LV2::world;

	SharedPtr<Engine> engine(new Engine(plugin->world));
	plugin->world->set_local_engine(engine);

	SharedPtr<QueuedEngineInterface> interface(
			new Ingen::QueuedEngineInterface(*plugin->world->local_engine(), event_queue_size));
	plugin->world->set_engine(interface);
	plugin->world->local_engine()->add_event_source(interface);

	Raul::Thread::get().set_context(THREAD_PRE_PROCESS);
	ThreadManager::single_threaded = true;

	// FIXME: fixed (or at least maximum) buffer size
	engine->set_driver(SharedPtr<Driver>(new LV2Driver(*engine.get(), rate, 4096)));

	engine->activate();
	ThreadManager::single_threaded = true;

	ProcessContext context(*engine.get());
	context.locate(0, UINT_MAX, 0);

	engine->post_processor()->set_end_time(UINT_MAX);

	// FIXME: don't load all plugins, only necessary ones
	plugin->world->engine()->load_plugins();
	interface->process(*engine->post_processor(), context, false);
	engine->post_processor()->process();

	plugin->world->parser()->parse_document(plugin->world,
			plugin->world->engine().get(), patch->filename);

	while (!interface->empty()) {
		interface->process(*engine->post_processor(), context, false);
		engine->post_processor()->process();
	}

	engine->deactivate();

	return (LV2_Handle)plugin;
}


static void
ingen_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	IngenPlugin* me = (IngenPlugin*)instance;
	LV2::LV2Driver* driver = (LV2::LV2Driver*)me->world->local_engine()->driver();
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
}


static void
ingen_run(LV2_Handle instance, uint32_t sample_count)
{
	IngenPlugin* me = (IngenPlugin*)instance;
	// FIXME: don't do this every call
	Raul::Thread::get().set_context(THREAD_PROCESS);
	((LV2::LV2Driver*)me->world->local_engine()->driver())->run(sample_count);
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
	me->world->set_local_engine(SharedPtr<Engine>());
	me->world->set_engine(SharedPtr<EngineInterface>());
	//ingen_world_free(me->world);
	free(instance);
}


static const void*
ingen_extension_data(const char* uri)
{
	return NULL;
}


/* Library Code */

namespace Ingen {
namespace LV2 {

LV2Patch::LV2Patch(const std::string& u, const std::string& f)
	: uri(u), filename(f)
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

} // namespace LV2
} // namespace Ingen


static void
init()
{
	Shared::set_bundle_path_from_code((void*)&init);

	using namespace LV2;

	//Shared::World* world = ingen_world_new(&conf, argc, argv);

	world = ingen_world_new(&conf, argc, argv);
	if (!world->load("ingen_serialisation")) {
		Raul::error << "Unable to load serialisation module" << std::endl;
		//ingen_world_free(world);
		return;
	}


	Serialisation::Parser::PatchRecords records(
			world->parser()->find_patches(world,
				Glib::filename_to_uri(Shared::bundle_file_path("manifest.ttl"))));

	for (Serialisation::Parser::PatchRecords::iterator i = records.begin();
			i != records.end(); ++i) {
		patches.push_back(SharedPtr<const LV2Patch>(
				new LV2Patch(i->uri.str(), i->filename)));
	}


	//ingen_world_free(world);

	initialised = true;
}


/* LV2 Library Interface */

extern "C" {

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	if (!LV2::initialised)
		init();

	if (index >= LV2::patches.size())
		return NULL;
	else
		return &LV2::patches[index]->descriptor;
}

}


} // extern "C"

