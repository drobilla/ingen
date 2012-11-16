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
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
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

#include "Buffer.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EnginePort.hpp"
#include "EventWriter.hpp"
#include "GraphImpl.hpp"
#include "PostProcessor.hpp"
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"

#define NS_INGEN "http://drobilla.net/ns/ingen#"
#define NS_RDF   "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS  "http://www.w3.org/2000/01/rdf-schema#"

/** Record of a graph in this bundle. */
struct LV2Graph {
	LV2Graph(const std::string& u, const std::string& f);

	const std::string uri;
	const std::string filename;
	LV2_Descriptor    descriptor;
};

/** Ingen LV2 library. */
class Lib {
public:
	explicit Lib(const char* bundle_path);

	typedef std::vector< SharedPtr<const LV2Graph> > Graphs;

	Graphs graphs;
};

namespace Ingen {
namespace Server {

class LV2Driver;

void signal_main(ProcessContext& context, LV2Driver* driver);

class LV2Driver : public Ingen::Server::Driver
                , public Ingen::AtomSink
{
public:
	LV2Driver(Engine& engine, SampleCount block_length, SampleCount sample_rate)
		: _engine(engine)
		, _main_sem(0)
		, _reader(engine.world()->uri_map(),
		          engine.world()->uris(),
		          engine.world()->log(),
		          engine.world()->forge(),
		          *engine.world()->interface().get())
		, _writer(engine.world()->uri_map(),
		          engine.world()->uris(),
		          *this)
		, _from_ui(block_length * sizeof(float))  // FIXME: size
		, _to_ui(block_length * sizeof(float))  // FIXME: size
		, _root_graph(NULL)
		, _notify_capacity(0)
		, _block_length(block_length)
		, _sample_rate(sample_rate)
		, _frame_time(0)
		, _to_ui_overflow_sem(0)
		, _to_ui_overflow(false)
	{}

	void pre_process_port(ProcessContext& context, EnginePort* port) {
		PortImpl* graph_port = port->graph_port();
		void*     buffer     = port->buffer();

		if (!graph_port->is_input() || !buffer) {
			return;
		}

		Buffer* const graph_buf = graph_port->buffer(0).get();
		if (graph_port->is_a(PortType::AUDIO) ||
		    graph_port->is_a(PortType::CV)) {
			memcpy(graph_buf->samples(),
			       buffer,
			       context.nframes() * sizeof(float));
		} else if (graph_port->is_a(PortType::CONTROL)) {
			graph_buf->samples()[0] = ((float*)buffer)[0];
		} else {
			LV2_Atom_Sequence* seq      = (LV2_Atom_Sequence*)buffer;
			bool               enqueued = false;
			URIs&              uris     = graph_port->bufs().uris();
			graph_buf->prepare_write(context);
			LV2_ATOM_SEQUENCE_FOREACH(seq, ev) {
				if (!graph_buf->append_event(
					    ev->time.frames, ev->body.size, ev->body.type,
					    (const uint8_t*)LV2_ATOM_BODY(&ev->body))) {
					_engine.log().warn("Failed to write to buffer, event lost!\n");
				}

				if (AtomReader::is_message(uris, &ev->body)) {
					enqueue_message(&ev->body);
					enqueued = true;
				}
			}

			if (enqueued) {
				_main_sem.post();
			}
		}
	}

	void post_process_port(ProcessContext& context, EnginePort* port) {
		PortImpl* graph_port = port->graph_port();
		void*     buffer     = port->buffer();

		if (graph_port->is_input() || !buffer) {
			return;
		}

		Buffer* graph_buf = graph_port->buffer(0).get();
		if (graph_port->is_a(PortType::AUDIO) ||
		    graph_port->is_a(PortType::CV)) {
			memcpy(buffer,
			       graph_buf->samples(),
			       context.nframes() * sizeof(float));
		} else if (graph_port->is_a(PortType::CONTROL)) {
			((float*)buffer)[0] = graph_buf->samples()[0];
		} else if (graph_port->index() != 1) {
			/* Copy Sequence output to LV2 buffer, except notify output which
			   is written by flush_to_ui() (TODO: merge) */
			memcpy(buffer,
			       graph_buf->atom(),
			       sizeof(LV2_Atom) + graph_buf->atom()->size);
		}
	}

	void run(uint32_t nframes) {
		_engine.process_context().locate(_frame_time, nframes);

		// Notify buffer is a Chunk with size set to the available space
		_notify_capacity = ((LV2_Atom_Sequence*)_ports[1]->buffer())->atom.size;

		for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i) {
			pre_process_port(_engine.process_context(), *i);
		}

		_engine.run(nframes);
		if (_engine.post_processor()->pending()) {
			_main_sem.post();
		}

		flush_to_ui(_engine.process_context());

		for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i) {
			post_process_port(_engine.process_context(), *i);
		}

		_frame_time += nframes;
	}

	virtual void deactivate() {
		_engine.quit();
		_main_sem.post();
	}

	virtual void       set_root_graph(GraphImpl* graph) { _root_graph = graph; }
	virtual GraphImpl* root_graph()                     { return _root_graph; }

	virtual EnginePort* get_port(const Raul::Path& path) {
		for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i) {
			if ((*i)->graph_port()->path() == path) {
				return *i;
			}
		}

		return NULL;
	}

	/** This does not have to be real-time since LV2 has no dynamic ports.
	 * It is only called on initial load.
	 */
	virtual void add_port(ProcessContext& context, EnginePort* port) {
		const uint32_t index = port->graph_port()->index();
		if (_ports.size() <= index) {
			_ports.resize(index + 1);
		}
		_ports[index] = port;
	}

	/** Unused since LV2 has no dynamic ports. */
	virtual void remove_port(ProcessContext& context, EnginePort* port) {}

	/** Unused since LV2 has no dynamic ports. */
	virtual void register_port(EnginePort& port) {}

	/** Unused since LV2 has no dynamic ports. */
	virtual void unregister_port(EnginePort& port) {}

	/** Unused since LV2 has no dynamic ports. */
	virtual void rename_port(const Raul::Path& old_path,
	                         const Raul::Path& new_path) {}

	virtual EnginePort* create_port(DuplexPort* graph_port) {
		return new EnginePort(graph_port);
	}

	virtual void append_time_events(ProcessContext& context,
	                                Buffer&         buffer)
	{}

	/** Called in run thread for events received at control input port. */
	void enqueue_message(const LV2_Atom* atom) {
		if (_from_ui.write(lv2_atom_total_size(atom), atom) == 0) {
#ifndef NDEBUG
			_engine.log().error("Control input buffer overflow\n");
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
				_engine.log().error("Error reading head from from-UI ring\n");
				break;
			}

			buf = realloc(buf, sizeof(LV2_Atom) + atom.size);
			memcpy(buf, &atom, sizeof(LV2_Atom));

			if (!_from_ui.read(atom.size, (char*)buf + sizeof(LV2_Atom))) {
				_engine.log().error("Error reading body from from-UI ring\n");
				break;
			}

			_reader.write((LV2_Atom*)buf);
			read += sizeof(LV2_Atom) + atom.size;
		}
		free(buf);
	}

	void flush_to_ui(ProcessContext& context) {
		if (_ports.size() < 2) {
			_engine.log().error("Standard control ports are not present\n");
			return;
		}

		LV2_Atom_Sequence* seq = (LV2_Atom_Sequence*)_ports[1]->buffer();
		if (!seq) {
			_engine.log().error("Notify output not connected\n");
			return;
		}

		// Initialise output port buffer to an empty Sequence
		seq->atom.type = _engine.world()->uris().atom_Sequence;
		seq->atom.size = sizeof(LV2_Atom_Sequence_Body);

		const uint32_t read_space = _to_ui.read_space();
		for (uint32_t read = 0; read < read_space;) {
			LV2_Atom atom;
			if (!_to_ui.peek(sizeof(LV2_Atom), &atom)) {
				_engine.log().error("Error reading head from to-UI ring\n");
				break;
			}

			if (seq->atom.size + sizeof(LV2_Atom) + atom.size > _notify_capacity) {
				std::cerr << "Notify overflow" << std::endl;
				break;  // Output port buffer full, resume next time
			}

			LV2_Atom_Event* ev = (LV2_Atom_Event*)(
				(uint8_t*)seq + lv2_atom_total_size(&seq->atom));

			ev->time.frames = 0;  // TODO: Time?
			ev->body        = atom;

			_to_ui.skip(sizeof(LV2_Atom));
			if (!_to_ui.read(ev->body.size, LV2_ATOM_BODY(&ev->body))) {
				_engine.log().error("Error reading body from to-UI ring\n");
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

	typedef std::vector<EnginePort*> Ports;

	Ports& ports() { return _ports; }

private:
	Engine&          _engine;
	Ports            _ports;
	Raul::Semaphore  _main_sem;
	AtomReader       _reader;
	AtomWriter       _writer;
	Raul::RingBuffer _from_ui;
	Raul::RingBuffer _to_ui;
	GraphImpl*       _root_graph;
	uint32_t         _notify_capacity;
	SampleCount      _block_length;
	SampleCount      _sample_rate;
	SampleCount      _frame_time;
	Raul::Semaphore  _to_ui_overflow_sem;
	bool             _to_ui_overflow;
};

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
		: Raul::Thread()
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

static Lib::Graphs
find_graphs(const Glib::ustring& manifest_uri)
{
	Sord::World      world;
	const Sord::URI  base(world, manifest_uri);
	const Sord::Node nil;
	const Sord::URI  ingen_Graph (world, NS_INGEN "Graph");
	const Sord::URI  rdf_type    (world, NS_RDF   "type");
	const Sord::URI  rdfs_seeAlso(world, NS_RDFS  "seeAlso");

	SerdEnv* env = serd_env_new(sord_node_to_serd_node(base.c_obj()));
	Sord::Model model(world, manifest_uri);
	model.load_file(env, SERD_TURTLE, manifest_uri);

	Lib::Graphs graphs;
	for (Sord::Iter i = model.find(nil, rdf_type, ingen_Graph); !i.end(); ++i) {
		const Sord::Node  graph     = i.get_subject();
		Sord::Iter        f         = model.find(graph, rdfs_seeAlso, nil);
		const std::string graph_uri = graph.to_c_string();
		if (!f.end()) {
			const uint8_t* file_uri  = f.get_object().to_u_string();
			uint8_t*       file_path = serd_file_uri_parse(file_uri, NULL);
			graphs.push_back(boost::shared_ptr<const LV2Graph>(
				                 new LV2Graph(graph_uri, (const char*)file_path)));
			free(file_path);
		}
	}

	serd_env_free(env);
	return graphs;
}

static LV2_Handle
ingen_instantiate(const LV2_Descriptor*    descriptor,
                  double                   rate,
                  const char*              bundle_path,
                  const LV2_Feature*const* features)
{
	// Get features from features array
	LV2_URID_Map*             map     = NULL;
	LV2_URID_Unmap*           unmap   = NULL;
	LV2_Log_Log*              log     = NULL;
	const LV2_Options_Option* options = NULL;
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			map = (LV2_URID_Map*)features[i]->data;
		} else if (!strcmp(features[i]->URI, LV2_URID__unmap)) {
			unmap = (LV2_URID_Unmap*)features[i]->data;
		} else if (!strcmp(features[i]->URI, LV2_LOG__log)) {
			log = (LV2_Log_Log*)features[i]->data;
		} else if (!strcmp(features[i]->URI, LV2_OPTIONS__options)) {
			options = (const LV2_Options_Option*)features[i]->data;
		}
	}

	if (!Glib::thread_supported()) {
		Glib::thread_init();
	}

	set_bundle_path(bundle_path);
	Lib::Graphs graphs = find_graphs(
		Glib::filename_to_uri(Ingen::bundle_file_path("manifest.ttl")));

	const LV2Graph* graph = NULL;
	for (Lib::Graphs::iterator i = graphs.begin(); i != graphs.end(); ++i) {
		if ((*i)->uri == descriptor->URI) {
			graph = (*i).get();
			break;
		}
	}

	if (!graph) {
		const std::string msg((Raul::fmt("Could not find graph %1%\n")
		                       % descriptor->URI).str());
		if (log) {
			log->vprintf(log->handle,
			             map->map(map->handle, LV2_LOG__Error),
			             msg.c_str(),
			             NULL);
		} else {
			std::cerr << msg.c_str() << std::endl;
		}
		return NULL;
	}

	IngenPlugin* plugin = new IngenPlugin();
	plugin->map = map;
	plugin->world = new Ingen::World(
		plugin->argc, plugin->argv, map, unmap, log);
	if (!plugin->world->load_module("serialisation")) {
		delete plugin->world;
		return NULL;
	}

	LV2_URID bufsz_max    = map->map(map->handle, LV2_BUF_SIZE__maxBlockLength);
	LV2_URID bufsz_seq    = map->map(map->handle, LV2_BUF_SIZE__sequenceSize);
	LV2_URID atom_Int     = map->map(map->handle, LV2_ATOM__Int);
	int32_t  block_length = 0;
	int32_t  seq_size     = 0;
	if (options) {
		for (const LV2_Options_Option* o = options; o->key; ++o) {
			if (o->key == bufsz_max && o->type == atom_Int) {
				block_length = *(const int32_t*)o->value;
			} else if (o->key == bufsz_seq && o->type == atom_Int) {
				seq_size = *(const int32_t*)o->value;
			}
		}
	}
	if (block_length == 0) {
		block_length = 4096;
		plugin->world->log().warn("No maximum block length given\n");
	}
	if (seq_size == 0) {
		seq_size = 16384;
		plugin->world->log().warn("No maximum sequence size given\n");
	}

	plugin->world->log().info(
		Raul::fmt("Block: %1% frames, Sequence: %2% bytes\n")
		% block_length % seq_size);

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
	                                    graph->filename);

	while (engine->pending_events()) {
		engine->process_events();
		engine->post_processor()->process();
	}

	/* Register client after loading graph so the to-ui ring does not overflow.
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
	} else {
		engine->log().error(Raul::fmt("Connect to non-existent port %1%\n")
		                    % port);
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
		plugin->world->log().error("Missing state:mapPath, state:makePath, or urid:Map\n");
		return LV2_STATE_ERR_NO_FEATURE;
	}

	LV2_URID ingen_file = plugin->map->map(plugin->map->handle, NS_INGEN "file");
	LV2_URID atom_Path = plugin->map->map(plugin->map->handle,
	                                      LV2_ATOM__Path);

	char* real_path  = make_path->path(make_path->handle, "graph.ttl");
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
		plugin->world->log().error("Missing state:mapPath\n");
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
		plugin->world->log().error("Failed to restore ingen:file\n");
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

LV2Graph::LV2Graph(const std::string& u, const std::string& f)
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
	graphs = find_graphs(
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
	return index < lib->graphs.size() ? &lib->graphs[index]->descriptor : NULL;
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
