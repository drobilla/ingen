/* Ingen.LV2 - A thin wrapper which allows Ingen to run as an LV2 plugin.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
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

#ifndef PLUGIN_URI
#error "This file requires PLUGIN_URI to be defined"
#endif

#include <stdlib.h>
#include "lv2.h"
#include "engine/Engine.hpp"
#include "engine/AudioDriver.hpp"
#include "engine/ProcessContext.hpp"
#include "engine/PatchImpl.hpp"
#include "module/World.hpp"

extern "C" {

using namespace Ingen;

/* Plugin */

struct IngenLV2Driver : public Ingen::AudioDriver {
	IngenLV2Driver(Engine& engine, SampleCount buffer_size, SampleCount sample_rate)
		: _context(engine)
		, _buffer_size(buffer_size)
		, _sample_rate(sample_rate)
		, _frame_time(0)
	{}

	void activate() {}
	void deactivate() {}
	bool is_activated() const { return true; }

	void run(uint32_t nframes) {
		_context.set_time_slice(nframes, _frame_time, _frame_time + nframes);
		if (_root_patch)
			_root_patch->process(_context);
		_frame_time += nframes;
	}
	
	virtual void       set_root_patch(PatchImpl* patch) {}
	virtual PatchImpl* root_patch()                     { return NULL; }
	
	virtual void        add_port(DriverPort* port)          {}
	virtual DriverPort* remove_port(const Raul::Path& path) { return NULL; }
	
	virtual DriverPort* create_port(DuplexPort* patch_port) { return NULL; }
	
	virtual DriverPort* driver_port(const Raul::Path& path) { return NULL; }
	
	virtual SampleCount buffer_size()  const { return _buffer_size; }
	virtual SampleCount sample_rate()  const { return _sample_rate; }
	virtual SampleCount frame_time()   const { return _frame_time;}

	virtual bool            is_realtime() const { return true; }
	virtual ProcessContext& context()           { return _context; }
	
private:
	ProcessContext _context;
	PatchImpl*     _root_patch;
	SampleCount    _buffer_size;
	SampleCount    _sample_rate;
	SampleCount    _frame_time;
};


struct IngenPlugin {
	Ingen::Shared::World*    world;
	SharedPtr<Ingen::Engine> engine;
};


static void
ingen_activate(LV2_Handle instance)
{
	IngenPlugin* plugin = (IngenPlugin*)instance;
	plugin->engine->activate(1);
}


static void
ingen_cleanup(LV2_Handle instance)
{
	IngenPlugin* plugin = (IngenPlugin*)instance;
	plugin->engine.reset();
	Ingen::Shared::destroy_world();
	free(instance);
}


static void
ingen_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	//IngenPlugin* plugin = (IngenPlugin*)instance;
}


static LV2_Handle
ingen_instantiate(const LV2_Descriptor*    descriptor,
                  double                   rate,
                  const char*              bundle_path,
                  const LV2_Feature*const* features)
{
	IngenPlugin* plugin = (IngenPlugin*)malloc(sizeof(IngenPlugin));

	Shared::bundle_path = bundle_path;

	plugin->world = Ingen::Shared::get_world();
	plugin->engine = SharedPtr<Engine>(new Engine(plugin->world));
	plugin->world->local_engine = plugin->engine;
	
	// FIXME: fixed buffer size
	plugin->engine->set_driver(DataType::AUDIO,
			SharedPtr<Driver>(new IngenLV2Driver(*plugin->engine, rate, 4096)));
	
	return (LV2_Handle)plugin;
}


static void
ingen_run(LV2_Handle instance, uint32_t sample_count)
{
	IngenPlugin* plugin = (IngenPlugin*)instance;
	((IngenLV2Driver*)plugin->engine->audio_driver())->run(sample_count);
}


static const void*
ingen_extension_data(const char* uri)
{
	return NULL;
}


static void
ingen_deactivate(LV2_Handle instance)
{
	IngenPlugin* plugin = (IngenPlugin*)instance;
	plugin->engine->deactivate();
}


/* Library */

static LV2_Descriptor *ingen_descriptor = NULL;

static void
init_descriptor()
{
	ingen_descriptor = (LV2_Descriptor*)malloc(sizeof(LV2_Descriptor));

	ingen_descriptor->URI = PLUGIN_URI;
	ingen_descriptor->instantiate = ingen_instantiate;
	ingen_descriptor->connect_port = ingen_connect_port;
	ingen_descriptor->activate = ingen_activate;
	ingen_descriptor->run = ingen_run;
	ingen_descriptor->deactivate = ingen_deactivate;
	ingen_descriptor->cleanup = ingen_cleanup;
	ingen_descriptor->extension_data = ingen_extension_data;
}


LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	if (!ingen_descriptor)
		init_descriptor();

	switch (index) {
	case 0:
		return ingen_descriptor;
	default:
		return NULL;
	}
}


} // extern "C"

