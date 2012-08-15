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

#include <stdlib.h>

#include <string>
#include <vector>

#include <glib.h>
#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <glibmm/timer.h>

#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include "ingen/Interface.hpp"
#include "ingen/serialisation/Parser.hpp"
#include "ingen/serialisation/Serialiser.hpp"
#include "ingen/AtomReader.hpp"
#include "ingen/AtomWriter.hpp"
#include "ingen/Store.hpp"
#include "ingen/World.hpp"
#include "ingen/runtime_paths.hpp"
#include "raul/Semaphore.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/Thread.hpp"
#include "raul/log.hpp"

#include "Buffer.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EnginePort.hpp"
#include "EventWriter.hpp"
#include "PatchImpl.hpp"
#include "PostProcessor.hpp"
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"

#define NS_INGEN "http://drobilla.net/ns/ingen#"
#define NS_RDF   "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS  "http://www.w3.org/2000/01/rdf-schema#"

/** Record of a patch in this bundle. */
struct LV2Patch {
	LV2Patch(const std::string& u, const std::string& f);

	const std::string uri;
	const std::string filename;
	LV2_Descriptor    descriptor;
};

/** Ingen LV2 library. */
class Lib {
public:
	explicit Lib(const char* bundle_path);

	typedef std::vector< SharedPtr<const LV2Patch> > Patches;

	Patches patches;
};

namespace Ingen {
namespace Server {

class LV2Driver;

void enqueue_message(
	ProcessContext& context, LV2Driver* driver, const LV2_Atom* msg);

void signal_main(ProcessContext& context, LV2Driver* driver);

class LV2Port : public EnginePort
{
public:
	LV2Port(LV2Driver* driver, DuplexPort* patch_port)
		: EnginePort(patch_port)
		, _driver(driver)
	{}

	void pre_process(ProcessContext& context) {
		if (!is_input() || !_buffer) {
			return;
		}

		Buffer* const patch_buf = _patch_port->buffer(0).get();
		if (_patch_port->is_a(PortType::AUDIO) ||
		    _patch_port->is_a(PortType::CV)) {
			memcpy(patch_buf->samples(),
			       _buffer,
			       context.nframes() * sizeof(float));
		} else if (_patch_port->is_a(PortType::CONTROL)) {
			patch_buf->samples()[0] = ((float*)_buffer)[0];
		} else {
			LV2_Atom_Sequence* seq      = (LV2_Atom_Sequence*)_buffer;
			bool               enqueued = false;
			URIs&              uris     = _patch_port->bufs().uris();
			patch_buf->prepare_write(context);
			LV2_ATOM_SEQUENCE_FOREACH(seq, ev) {
				if (!patch_buf->append_event(
					    ev->time.frames, ev->body.size, ev->body.type,
					    (const uint8_t*)LV2_ATOM_BODY(&ev->body))) {
					Raul::warn("Failed to write to buffer, event lost!\n");
				}

				if (AtomReader::is_message(uris, &ev->body)) {
					enqueue_message(context, _driver, &ev->body);
					enqueued = true;
				}
			}

			if (enqueued) {
				signal_main(context, _driver);
			}
		}
	}

	void post_process(ProcessContext& context) {
		if (is_input() || !_buffer) {
			return;
		}

		Buffer* patch_buf = _patch_port->buffer(0).get();
		if (_patch_port->is_a(PortType::AUDIO) ||
		    _patch_port->is_a(PortType::CV)) {
			memcpy(_buffer,
			       patch_buf->samples(),
			       context.nframes() * sizeof(float));
		} else if (_patch_port->is_a(PortType::CONTROL)) {
			((float*)_buffer)[0] = patch_buf->samples()[0];
		} else {
			memcpy(_buffer,
			       patch_buf->atom(),
			       sizeof(LV2_Atom) + patch_buf->atom()->size);
		}
	}

private:
	LV2Driver* _driver;
};

class LV2Driver : public Ingen::Server::Driver
                , public Ingen::AtomSink
{
private:
	typedef std::vector<LV2Port*> Ports;

public:
	LV2Driver(Engine& engine, SampleCount block_length, SampleCount sample_rate)
		: _engine(engine)
		, _main_sem(0)
		, _reader(engine.world()->uri_map(),
		          engine.world()->uris(),
		          engine.world()->forge(),
		          *engine.world()->interface().get())
		, _writer(engine.world()->uri_map(),
		          engine.world()->uris(),
		          *this)
		, _from_ui(block_length * sizeof(float))  // FIXME: size
		, _to_ui(block_length * sizeof(float))  // FIXME: size
		, _root_patch(NULL)
		, _block_length(block_length)
		, _sample_rate(sample_rate)
		, _frame_time(0)
		, _to_ui_overflow_sem(0)
		, _to_ui_overflow(false)
	{}

	void run(uint32_t nframes) {
		_engine.process_context().locate(_frame_time, nframes);

		for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i)
			(*i)->pre_process(_engine.process_context());

		_engine.run(nframes);
		if (_engine.post_processor()->pending()) {
			_main_sem.post();
		}

		flush_to_ui();

		for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i)
			(*i)->post_process(_engine.process_context());

		_frame_time += nframes;
	}

	virtual void deactivate() {
		_engine.quit();
		_main_sem.post();
	}

	virtual void       set_root_patch(PatchImpl* patch) { _root_patch = patch; }
	virtual PatchImpl* root_patch()                     { return _root_patch; }

	virtual EnginePort* engine_port(ProcessContext&   context,
	                                const Raul::Path& path) {
		for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i) {
			if ((*i)->patch_port()->path() == path) {
				return (*i);
			}
		}

		return NULL;
	}

	EnginePort* port(const Raul::Path& path) { return NULL; }

	/** Doesn't have to be real-time safe since LV2 has no dynamic ports. */
	virtual void add_port(ProcessContext& context, EnginePort* port) {
		assert(dynamic_cast<LV2Port*>(port));
		assert(port->patch_port()->index() == _ports.size());
		_ports.push_back((LV2Port*)port);
	}

	/** Doesn't have to be real-time safe since LV2 has no dynamic ports. */
	virtual Raul::Deletable* remove_port(ProcessContext& context,
	                                     EnginePort*     port) {
		for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i) {
			if (*i == port) {
				_ports.erase(i);
				return NULL;
			}
		}

		return NULL;
	}

	/** UNused since LV2 has no dynamic ports. */
	virtual void rename_port(const Raul::Path& old_path,
	                         const Raul::Path& new_path) {}

	virtual EnginePort* create_port(DuplexPort* patch_port) {
		return new LV2Port(this, patch_port);
	}

	/** Called in run thread for events received at control input port. */
	void enqueue_message(const LV2_Atom* atom) {
		if (_from_ui.write(lv2_atom_total_size(atom), atom) == 0) {
#ifndef NDEBUG
			Raul::error << "Control input buffer overflow" << std::endl;
#endif
		}
	}

	Raul::Semaphore& main_sem() { return _main_sem; }

	/** AtomSink::write implementation called by the PostProcessor in the main
	 * thread to write responses to the UI.
	 */
	bool write(const LV2_Atom* atom) {
		// Called from post-processor in main thread
		while (_to_ui.write(lv2_atom_total_size(atom), atom) == 0) {
			// Overflow, wait until ring is drained next cycle
			_to_ui_overflow = true;
			_to_ui_overflow_sem.wait();
			_to_ui_overflow = false;
		}
		return true;
	}

	void consume_from_ui() {
		const uint32_t read_space = _from_ui.read_space();
		void*          buf        = NULL;
		for (uint32_t read = 0; read < read_space;) {
			LV2_Atom atom;
			if (!_from_ui.read(sizeof(LV2_Atom), &atom)) {
				Raul::error << "Error reading head from from-UI ring" << std::endl;
				break;
			}

			buf = realloc(buf, sizeof(LV2_Atom) + atom.size);
			memcpy(buf, &atom, sizeof(LV2_Atom));

			if (!_from_ui.read(atom.size, (char*)buf + sizeof(LV2_Atom))) {
				Raul::error << "Error reading body from from-UI ring" << std::endl;
				break;
			}

			_reader.write((LV2_Atom*)buf);
			read += sizeof(LV2_Atom) + atom.size;
		}
		free(buf);
	}

	void flush_to_ui() {
		assert(_ports.size() >= 2);

		LV2_Atom_Sequence* seq = (LV2_Atom_Sequence*)_ports[1]->buffer();
		if (!seq) {
			Raul::error << "Control out port not connected" << std::endl;
			return;
		}

		// Output port buffer is a Chunk with size set to the available space
		const uint32_t capacity = seq->atom.size;

		// Initialise output port buffer to an empty Sequence
		seq->atom.type = _engine.world()->uris().atom_Sequence;
		seq->atom.size = sizeof(LV2_Atom_Sequence_Body);

		const uint32_t read_space = _to_ui.read_space();
		for (uint32_t read = 0; read < read_space;) {
			LV2_Atom atom;
			if (!_to_ui.peek(sizeof(LV2_Atom), &atom)) {
				Raul::error << "Error reading head from to-UI ring" << std::endl;
				break;
			}

			if (seq->atom.size + sizeof(LV2_Atom) + atom.size > capacity) {
				break;  // Output port buffer full, resume next time
			}

			LV2_Atom_Event* ev = (LV2_Atom_Event*)(
				(uint8_t*)seq + lv2_atom_total_size(&seq->atom));

			ev->time.frames = 0;  // TODO: Time?
			ev->body        = atom;

			_to_ui.skip(sizeof(LV2_Atom));
			if (!_to_ui.read(ev->body.size, LV2_ATOM_BODY(&ev->body))) {
				Raul::error << "Error reading body from to-UI ring" << std::endl;
				break;
			}

			read           += lv2_atom_total_size(&ev->body);
			seq->atom.size += sizeof(LV2_Atom_Event) + ev->body.size;
		}

		_to_ui_overflow_sem.post();
	}

	virtual SampleCount block_length() const { return _block_length; }
	virtual SampleCount sample_rate()  const { return _sample_rate; }
	virtual SampleCount frame_time()   const { return _frame_time; }

	virtual bool is_realtime() const { return true; }
	AtomReader&  reader()            { return _reader; }
	AtomWriter&  writer()            { return _writer; }

	Ports& ports() { return _ports; }

private:
	Engine&          _engine;
	Raul::Semaphore  _main_sem;
	AtomReader       _reader;
	AtomWriter       _writer;
	Raul::RingBuffer _from_ui;
	Raul::RingBuffer _to_ui;
	PatchImpl*       _root_patch;
	SampleCount      _block_length;
	SampleCount      _sample_rate;
	SampleCount      _frame_time;
	Ports            _ports;
	Raul::Semaphore  _to_ui_overflow_sem;
	bool             _to_ui_overflow;
};

void
enqueue_message(ProcessContext& context, LV2Driver* driver, const LV2_Atom* msg)
{
	driver->enqueue_message(msg);
}

void
signal_main(ProcessContext& context, LV2Driver* driver)
{
	driver->main_sem().post();
}

} // namespace Server
} // namespace Ingen

extern "C" {

using namespace Ingen;
using namespace Ingen::Server;

class MainThread : public Raul::Thread
{
public:
	explicit MainThread(SharedPtr<Engine> engine,
	                    LV2Driver*        driver)
		: Raul::Thread("Main")
		, _engine(engine)
		, _driver(driver)
	{}

private:
	virtual void _run() {
		while (true) {
			// Wait until there is work to be done
			_driver->main_sem().wait();

			// Convert pending messages to events and push to pre processor
			_driver->consume_from_ui();

			// Run post processor and maid to finalise events from last time
			if (!_engine->main_iteration()) {
				return;
			}
		}
	}

	SharedPtr<Engine> _engine;
	LV2Driver*        _driver;
};

struct IngenPlugin {
	IngenPlugin()
		: world(NULL)
		, main(NULL)
		, map(NULL)
		, argc(0)
		, argv(NULL)
	{}

	Ingen::World* world;
	MainThread*   main;
	LV2_URID_Map* map;
	int           argc;
	char**        argv;
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
			Raul::error << "[Ingen] Patch has no rdfs:seeAlso" << std::endl;
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
	if (!Glib::thread_supported()) {
		Glib::thread_init();
	}

	set_bundle_path(bundle_path);
	Lib::Patches patches = find_patches(
		Glib::filename_to_uri(Ingen::bundle_file_path("manifest.ttl")));

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

	IngenPlugin*         plugin = new IngenPlugin();
	LV2_URID_Unmap*      unmap  = NULL;
	LV2_Buf_Size_Access* access = NULL;
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			plugin->map = (LV2_URID_Map*)features[i]->data;
		} else if (!strcmp(features[i]->URI, LV2_URID__unmap)) {
			unmap = (LV2_URID_Unmap*)features[i]->data;
		} else if (!strcmp(features[i]->URI, LV2_BUF_SIZE__access)) {
			access = (LV2_Buf_Size_Access*)features[i]->data;
		}
	}

	plugin->world = new Ingen::World(
		plugin->argc, plugin->argv, plugin->map, unmap);
	if (!plugin->world->load_module("serialisation")) {
		delete plugin->world;
		return NULL;
	}

	uint32_t block_length = 4096;
	uint32_t seq_size     = 0;
	if (access) {
		uint32_t min, multiple_of, power_of;
		access->get_sample_count(
			access->handle, &min, &block_length, &multiple_of, &power_of);
		access->get_buf_size(
			access->handle, &seq_size,
			plugin->world->uris().atom_Sequence, block_length);
		Raul::info(Raul::fmt("Block length: %1% frames\n") % block_length);
		Raul::info(Raul::fmt("Sequence size: %1% bytes\n") % seq_size);
	} else {
		Raul::warn("Warning: No buffer size access, guessing 4096 frames.\n");
	}

	SharedPtr<Server::Engine> engine(new Server::Engine(plugin->world));
	plugin->world->set_engine(engine);

	SharedPtr<EventWriter> interface =
		SharedPtr<EventWriter>(engine->interface(), NullDeleter<EventWriter>);

	plugin->world->set_interface(interface);

	Server::ThreadManager::set_flag(Server::THREAD_PRE_PROCESS);
	Server::ThreadManager::single_threaded = true;

	LV2Driver* driver = new LV2Driver(*engine.get(), block_length, rate);
	engine->set_driver(SharedPtr<Ingen::Server::Driver>(driver));

	plugin->main = new MainThread(engine, driver);

	engine->activate();
	Server::ThreadManager::single_threaded = true;

	engine->process_context().locate(0, block_length);

	engine->post_processor()->set_end_time(block_length);
	engine->process_events();
	engine->post_processor()->process();

	plugin->world->parser()->parse_file(plugin->world,
	                                    plugin->world->interface().get(),
	                                    patch->filename);

	while (engine->pending_events()) {
		engine->process_events();
		engine->post_processor()->process();
	}

	/* Register client after loading patch so the to-ui ring does not overflow.
	   Since we are not yet rolling, it won't be drained, causing a deadlock. */
	SharedPtr<Interface> client(&driver->writer(), NullDeleter<Interface>);
	interface->set_respondee(client);
	engine->register_client(Raul::URI("ingen:lv2"), client);

	return (LV2_Handle)plugin;
}

static void
ingen_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	using namespace Ingen::Server;

	IngenPlugin*    me     = (IngenPlugin*)instance;
	Server::Engine* engine = (Server::Engine*)me->world->engine().get();
	LV2Driver*      driver = (LV2Driver*)engine->driver();
	if (port < driver->ports().size()) {
		driver->ports().at(port)->set_buffer(data);
		assert(driver->ports().at(port)->patch_port()->index() == port);
		assert(driver->ports().at(port)->buffer() == data);
	} else {
		Raul::warn << "Connect to non-existent port " << port << std::endl;
	}
}

static void
ingen_activate(LV2_Handle instance)
{
	IngenPlugin* me = (IngenPlugin*)instance;
	me->world->engine()->activate();
	//((EventWriter*)me->world->engine().get())->start();
	me->main->start();
}

static void
ingen_run(LV2_Handle instance, uint32_t sample_count)
{
	IngenPlugin*    me     = (IngenPlugin*)instance;
	Server::Engine* engine = (Server::Engine*)me->world->engine().get();
	LV2Driver*      driver = (LV2Driver*)engine->driver();

	Server::ThreadManager::set_flag(Ingen::Server::THREAD_PROCESS);
	Server::ThreadManager::set_flag(Ingen::Server::THREAD_IS_REAL_TIME);

	driver->run(sample_count);
}

static void
ingen_deactivate(LV2_Handle instance)
{
	IngenPlugin* me = (IngenPlugin*)instance;
	me->world->engine()->deactivate();
}

static void
ingen_cleanup(LV2_Handle instance)
{
	IngenPlugin* me = (IngenPlugin*)instance;
	me->world->set_engine(SharedPtr<Ingen::Server::Engine>());
	me->world->set_interface(SharedPtr<Ingen::Interface>());
	delete me->world;
	delete me;
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

static LV2_State_Status
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
		            << std::endl;
		return LV2_STATE_ERR_NO_FEATURE;
	}

	LV2_URID ingen_file = plugin->map->map(plugin->map->handle, NS_INGEN "file");
	LV2_URID atom_Path = plugin->map->map(plugin->map->handle,
	                                      LV2_ATOM__Path);

	char* real_path  = make_path->path(make_path->handle, "patch.ttl");
	char* state_path = map_path->abstract_path(map_path->handle, real_path);

	Ingen::Store::iterator root = plugin->world->store()->find(Raul::Path("/"));
	plugin->world->serialiser()->to_file(root->second, real_path);

	store(handle,
	      ingen_file,
	      state_path,
	      strlen(state_path) + 1,
	      atom_Path,
	      LV2_STATE_IS_POD);

	free(state_path);
	free(real_path);
	return LV2_STATE_SUCCESS;
}

static LV2_State_Status
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
		Raul::error << "Missing state:mapPath" << std::endl;
		return LV2_STATE_ERR_NO_FEATURE;
	}

	LV2_URID ingen_file = plugin->map->map(plugin->map->handle, NS_INGEN "file");
	size_t   size;
	uint32_t type;
	uint32_t valflags;

	const void* path = retrieve(handle,
	                            ingen_file,
	                            &size, &type, &valflags);

	if (!path) {
		Raul::error << "Failed to restore ingen:file" << std::endl;
		return LV2_STATE_ERR_NO_PROPERTY;
	}

	const char* state_path = (const char*)path;
	char* real_path = map_path->absolute_path(map_path->handle, state_path);

	plugin->world->parser()->parse_file(plugin->world,
	                                    plugin->world->interface().get(),
	                                    real_path);
	free(real_path);
	return LV2_STATE_SUCCESS;
}

static const void*
ingen_extension_data(const char* uri)
{
	static const LV2_State_Interface state = { ingen_save, ingen_restore };
	if (!strcmp(uri, LV2_STATE__interface)) {
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
	Ingen::set_bundle_path(bundle_path);
	patches = find_patches(
		Glib::filename_to_uri(Ingen::bundle_file_path("manifest.ttl")));
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
