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

#include "Buffer.hpp"
#include "BufferRef.hpp"
#include "Driver.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "EnginePort.hpp"
#include "PortType.hpp"
#include "PostProcessor.hpp"
#include "RunContext.hpp"
#include "ThreadManager.hpp"
#include "types.hpp"

#include "ingen/AtomReader.hpp"
#include "ingen/AtomSink.hpp"
#include "ingen/AtomWriter.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/EngineBase.hpp"
#include "ingen/FilePath.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/Node.hpp"
#include "ingen/Parser.hpp"
#include "ingen/Serialiser.hpp"
#include "ingen/Store.hpp"
#include "ingen/URI.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "ingen/ingen.h"
#include "ingen/memory.hpp"
#include "ingen/runtime_paths.hpp"
#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"
#include "lv2/buf-size/buf-size.h"
#include "lv2/core/lv2.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/options/options.h"
#include "lv2/state/state.h"
#include "lv2/urid/urid.h"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"
#include "raul/RingBuffer.hpp"
#include "raul/Semaphore.hpp"
#include "raul/Symbol.hpp"
#include "serd/serd.h"
#include "sord/sordmm.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace ingen {

class Atom;

namespace server {

class GraphImpl;

/** Record of a graph in this bundle. */
struct LV2Graph : public Parser::ResourceRecord {
	explicit LV2Graph(Parser::ResourceRecord record);

	LV2_Descriptor descriptor;
};

/** Ingen LV2 library. */
class Lib {
public:
	explicit Lib(const char* bundle_path);

	using Graphs = std::vector<std::shared_ptr<const LV2Graph>>;

	Graphs graphs;
};

inline size_t
ui_ring_size(SampleCount block_length)
{
	return std::max(static_cast<size_t>(8192u),
	                static_cast<size_t>(block_length) * 16u);
}

class LV2Driver : public Driver, public ingen::AtomSink
{
public:
	LV2Driver(Engine&     engine,
	          SampleCount block_length,
	          size_t      seq_size,
	          SampleCount sample_rate)
		: _engine(engine)
		, _main_sem(0)
		, _reader(engine.world().uri_map(),
		          engine.world().uris(),
		          engine.world().log(),
		          *engine.world().interface())
		, _writer(engine.world().uri_map(),
		          engine.world().uris(),
		          *this)
		, _from_ui(ui_ring_size(block_length))
		, _to_ui(ui_ring_size(block_length))
		, _root_graph(nullptr)
		, _notify_capacity(0)
		, _block_length(block_length)
		, _seq_size(seq_size)
		, _sample_rate(sample_rate)
		, _frame_time(0)
		, _to_ui_overflow_sem(0)
		, _to_ui_overflow(false)
		, _instantiated(false)
	{}

	bool dynamic_ports() const override { return !_instantiated; }

	void pre_process_port(RunContext& ctx, EnginePort* port) {
		const URIs&       uris       = _engine.world().uris();
		const SampleCount nframes    = ctx.nframes();
		DuplexPort*       graph_port = port->graph_port();
		Buffer*           graph_buf  = graph_port->buffer(0).get();
		void*             lv2_buf    = port->buffer();

		if (graph_port->is_a(PortType::AUDIO) || graph_port->is_a(PortType::CV)) {
			graph_port->set_driver_buffer(lv2_buf, nframes * sizeof(float));
		} else if (graph_port->buffer_type() == uris.atom_Sequence) {
			graph_port->set_driver_buffer(lv2_buf,
			                              lv2_atom_total_size(
			                                  static_cast<LV2_Atom*>(lv2_buf)));

			if (graph_port->symbol() == "control") {  // TODO: Safe to use index?
				auto* seq = reinterpret_cast<LV2_Atom_Sequence*>(lv2_buf);

				bool enqueued = false;
				LV2_ATOM_SEQUENCE_FOREACH(seq, ev)
				{
					if (AtomReader::is_message(uris, &ev->body)) {
						enqueued = enqueue_message(&ev->body) || enqueued;
					}
				}

				if (enqueued) {
					// Enqueued a message for processing, raise semaphore
					_main_sem.post();
				}
			}
		}

		if (graph_port->is_input()) {
			graph_port->monitor(ctx);
		} else {
			graph_buf->prepare_write(ctx);
		}
	}

	static void post_process_port(RunContext&, EnginePort* port) {
		DuplexPort* graph_port = port->graph_port();

		// No copying necessary, host buffers are used directly
		// Reset graph port buffer pointer to no longer point to the Jack buffer
		if (graph_port->is_driver_port()) {
			graph_port->set_driver_buffer(nullptr, 0);
		}
	}

	void run(uint32_t nframes) {
		_engine.locate(_frame_time, nframes);

		// Notify buffer is a Chunk with size set to the available space
		_notify_capacity =
		    static_cast<LV2_Atom_Sequence*>(_ports[1]->buffer())->atom.size;

		for (auto& p : _ports) {
			pre_process_port(_engine.run_context(), p);
		}

		_engine.run(nframes);
		if (_engine.post_processor()->pending()) {
			_main_sem.post();
		}

		flush_to_ui(_engine.run_context());

		for (auto& p : _ports) {
			post_process_port(_engine.run_context(), p);
		}

		_frame_time += nframes;
	}

	void deactivate() override {
		_engine.quit();
		_main_sem.post();
	}

	virtual void       set_root_graph(GraphImpl* graph) { _root_graph = graph; }
	virtual GraphImpl* root_graph()                     { return _root_graph; }

	EnginePort* get_port(const raul::Path& path) override {
		for (auto& p : _ports) {
			if (p->graph_port()->path() == path) {
				return p;
			}
		}

		return nullptr;
	}

	/** Add a port.  Called only during init or restore. */
	void add_port(RunContext&, EnginePort* port) override {
		const uint32_t index = port->graph_port()->index();
		if (_ports.size() <= index) {
			_ports.resize(index + 1);
		}
		_ports[index] = port;
	}

	/** Remove a port.  Called only during init or restore. */
	void remove_port(RunContext&, EnginePort* port) override {
		const uint32_t index = port->graph_port()->index();
		_ports[index] = nullptr;
	}

	/** Unused since LV2 has no dynamic ports. */
	void register_port(EnginePort& port) override {}

	/** Unused since LV2 has no dynamic ports. */
	void unregister_port(EnginePort& port) override {}

	/** Unused since LV2 has no dynamic ports. */
	void rename_port(const raul::Path& old_path,
	                 const raul::Path& new_path) override {}

	/** Unused since LV2 has no dynamic ports. */
	void port_property(const raul::Path& path,
	                   const URI&        uri,
	                   const Atom&       value) override {}

	EnginePort* create_port(DuplexPort* graph_port) override {
		graph_port->set_is_driver_port(*_engine.buffer_factory());
		return new EnginePort(graph_port);
	}

	void append_time_events(RunContext&, Buffer& buffer) override {
		const URIs& uris = _engine.world().uris();
		auto*       seq  = static_cast<LV2_Atom_Sequence*>(_ports[0]->buffer());

		LV2_ATOM_SEQUENCE_FOREACH(seq, ev) {
			if (ev->body.type == uris.atom_Object) {
				const LV2_Atom_Object* obj =
				    reinterpret_cast<LV2_Atom_Object*>(&ev->body);

				if (obj->body.otype == uris.time_Position) {
					buffer.append_event(ev->time.frames,
					                    ev->body.size,
					                    ev->body.type,
					                    reinterpret_cast<const uint8_t*>(&ev->body + 1));
				}
			}
		}
	}

	int real_time_priority() override { return 60; }

	/** Called in run thread for events received at control input port. */
	bool enqueue_message(const LV2_Atom* atom) {
		if (_from_ui.write(lv2_atom_total_size(atom), atom) == 0) {
#ifndef NDEBUG
			_engine.log().error("Control input buffer overflow\n");
#endif
			return false;
		}
		return true;
	}

	raul::Semaphore& main_sem() { return _main_sem; }

	/** AtomSink::write implementation called by the PostProcessor in the main
	 * thread to write responses to the UI.
	 */
	bool write(const LV2_Atom* atom, int32_t default_id) override {
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
		void*          buf        = nullptr;
		for (uint32_t read = 0; read < read_space;) {
			LV2_Atom atom;
			if (!_from_ui.read(sizeof(LV2_Atom), &atom)) {
				_engine.log().rt_error("Error reading head from from-UI ring\n");
				break;
			}

			buf = realloc(buf, sizeof(LV2_Atom) + atom.size);
			memcpy(buf, &atom, sizeof(LV2_Atom));

			if (!_from_ui.read(atom.size,
			                   static_cast<char*>(buf) + sizeof(LV2_Atom))) {
				_engine.log().rt_error(
				    "Error reading body from from-UI ring\n");
				break;
			}

			_reader.write(static_cast<LV2_Atom*>(buf));
			read += sizeof(LV2_Atom) + atom.size;
		}
		free(buf);
	}

	void flush_to_ui(RunContext&) {
		if (_ports.size() < 2) {
			_engine.log().rt_error("Standard control ports are not present\n");
			return;
		}

		auto* seq = static_cast<LV2_Atom_Sequence*>(_ports[1]->buffer());
		if (!seq) {
			_engine.log().rt_error("Notify output not connected\n");
			return;
		}

		// Initialise output port buffer to an empty Sequence
		seq->atom.type = _engine.world().uris().atom_Sequence;
		seq->atom.size = sizeof(LV2_Atom_Sequence_Body);

		const uint32_t read_space = _to_ui.read_space();
		for (uint32_t read = 0; read < read_space;) {
			LV2_Atom atom;
			if (!_to_ui.peek(sizeof(LV2_Atom), &atom)) {
				_engine.log().rt_error("Error reading head from to-UI ring\n");
				break;
			}

			if (seq->atom.size + lv2_atom_pad_size(
				    sizeof(LV2_Atom_Event) + atom.size)
			    > _notify_capacity) {
				break;  // Output port buffer full, resume next time
			}

			auto* ev = reinterpret_cast<LV2_Atom_Event*>(
				reinterpret_cast<uint8_t*>(seq) +
				lv2_atom_total_size(&seq->atom));

			ev->time.frames = 0;  // TODO: Time?
			ev->body        = atom;

			_to_ui.skip(sizeof(LV2_Atom));
			if (!_to_ui.read(ev->body.size, LV2_ATOM_BODY(&ev->body))) {
				_engine.log().rt_error("Error reading body from to-UI ring\n");
				break;
			}

			read           += lv2_atom_total_size(&ev->body);
			seq->atom.size += lv2_atom_pad_size(
				sizeof(LV2_Atom_Event) + ev->body.size);
		}

		if (_to_ui_overflow) {
			_to_ui_overflow_sem.post();
		}
	}

	SampleCount block_length() const override { return _block_length; }
	size_t      seq_size()     const override { return _seq_size; }
	SampleCount sample_rate()  const override { return _sample_rate; }
	SampleCount frame_time()   const override { return _frame_time; }

	AtomReader& reader() { return _reader; }
	AtomWriter& writer() { return _writer; }

	using Ports = std::vector<EnginePort*>;

	Ports& ports() { return _ports; }

	void set_instantiated(bool instantiated) { _instantiated = instantiated; }

private:
	Engine&          _engine;
	Ports            _ports;
	raul::Semaphore  _main_sem;
	AtomReader       _reader;
	AtomWriter       _writer;
	raul::RingBuffer _from_ui;
	raul::RingBuffer _to_ui;
	GraphImpl*       _root_graph;
	uint32_t         _notify_capacity;
	SampleCount      _block_length;
	size_t           _seq_size;
	SampleCount      _sample_rate;
	SampleCount      _frame_time;
	raul::Semaphore  _to_ui_overflow_sem;
	bool             _to_ui_overflow;
	bool             _instantiated;
};

struct IngenPlugin {
	std::unique_ptr<World>       world;
	std::shared_ptr<Engine>      engine;
	std::unique_ptr<std::thread> main;
	LV2_URID_Map*                map  = nullptr;
	int                          argc = 0;
	char**                       argv = nullptr;
};

extern "C" {

static void
ingen_lv2_main(const std::shared_ptr<Engine>&    engine,
               const std::shared_ptr<LV2Driver>& driver)
{
	while (true) {
		// Wait until there is work to be done
		driver->main_sem().wait();

		// Convert pending messages to events and push to pre processor
		driver->consume_from_ui();

		// Run post processor and maid to finalise events from last time
		if (!engine->main_iteration()) {
			return;
		}
	}
}

static Lib::Graphs
find_graphs(const URI& manifest_uri)
{
	Sord::World world;
	Parser      parser;

	const std::set<Parser::ResourceRecord> resources = parser.find_resources(
		world,
		manifest_uri,
		URI(INGEN__Graph));

	Lib::Graphs graphs;
	for (const auto& r : resources) {
		graphs.push_back(std::make_shared<LV2Graph>(r));
	}

	return graphs;
}

static LV2_Handle
ingen_instantiate(const LV2_Descriptor*    descriptor,
                  double                   rate,
                  const char*              bundle_path,
                  const LV2_Feature*const* features)
{
	// Get features from features array
	LV2_URID_Map*             map     = nullptr;
	LV2_URID_Unmap*           unmap   = nullptr;
	LV2_Log_Log*              log     = nullptr;
	const LV2_Options_Option* options = nullptr;
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			map = static_cast<LV2_URID_Map*>(features[i]->data);
		} else if (!strcmp(features[i]->URI, LV2_URID__unmap)) {
			unmap = static_cast<LV2_URID_Unmap*>(features[i]->data);
		} else if (!strcmp(features[i]->URI, LV2_LOG__log)) {
			log = static_cast<LV2_Log_Log*>(features[i]->data);
		} else if (!strcmp(features[i]->URI, LV2_OPTIONS__options)) {
			options = static_cast<const LV2_Options_Option*>(features[i]->data);
		}
	}

	LV2_Log_Logger logger;
	lv2_log_logger_init(&logger, map, log);

	if (!map) {
		lv2_log_error(&logger, "host did not provide URI map feature\n");
		return nullptr;
	} else if (!unmap) {
		lv2_log_error(&logger, "host did not provide URI unmap feature\n");
		return nullptr;
	}

	set_bundle_path(bundle_path);
	const std::string manifest_path = ingen::bundle_file_path("manifest.ttl");
	SerdNode          manifest_node =
	    serd_node_new_file_uri(reinterpret_cast<const uint8_t*>(
	                               manifest_path.c_str()),
	                           nullptr,
	                           nullptr,
	                           true);

	Lib::Graphs graphs = find_graphs(URI(reinterpret_cast<const char*>(manifest_node.buf)));
	serd_node_free(&manifest_node);

	const LV2Graph* graph = nullptr;
	for (const auto& g : graphs) {
		if (g->uri == descriptor->URI) {
			graph = g.get();
			break;
		}
	}

	if (!graph) {
		lv2_log_error(&logger, "could not find graph <%s>\n", descriptor->URI);
		return nullptr;
	}

	auto* plugin = new IngenPlugin();
	plugin->map   = map;
	plugin->world = make_unique<ingen::World>(map, unmap, log);
	plugin->world->load_configuration(plugin->argc, plugin->argv);

	LV2_URID bufsz_max    = map->map(map->handle, LV2_BUF_SIZE__maxBlockLength);
	LV2_URID bufsz_seq    = map->map(map->handle, LV2_BUF_SIZE__sequenceSize);
	LV2_URID atom_Int     = map->map(map->handle, LV2_ATOM__Int);
	int32_t  block_length = 0;
	int32_t  seq_size     = 0;
	if (options) {
		for (const LV2_Options_Option* o = options; o->key; ++o) {
			if (o->key == bufsz_max && o->type == atom_Int) {
				block_length = *static_cast<const int32_t*>(o->value);
			} else if (o->key == bufsz_seq && o->type == atom_Int) {
				seq_size = *static_cast<const int32_t*>(o->value);
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
		"Block: %1% frames, Sequence: %2% bytes\n",
		block_length, seq_size);
	plugin->world->conf().set(
		"queue-size",
		plugin->world->forge().make(std::max(block_length, seq_size) * 4));

	auto engine = std::make_shared<Engine>(*plugin->world);
	plugin->engine = engine;
	plugin->world->set_engine(engine);

	std::shared_ptr<Interface> interface = engine->interface();

	plugin->world->set_interface(interface);

	ThreadManager::set_flag(THREAD_PRE_PROCESS);
	ThreadManager::single_threaded = true;

	auto* driver = new LV2Driver(*engine, block_length, seq_size, rate);
	engine->set_driver(std::shared_ptr<Driver>(driver));

	engine->activate();
	ThreadManager::single_threaded = true;

	std::lock_guard<std::mutex> lock(plugin->world->rdf_mutex());

	// Locate to time 0 to process initialization events
	engine->locate(0, block_length);
	engine->post_processor()->set_end_time(block_length);

	// Parse graph, filling the queue with events to create it
	plugin->world->interface()->bundle_begin();
	plugin->world->parser()->parse_file(*plugin->world,
	                                    *plugin->world->interface(),
	                                    graph->filename);
	plugin->world->interface()->bundle_end();

	// Drain event queue
	while (engine->pending_events()) {
		engine->process_all_events();
		engine->post_processor()->process();
		engine->maid()->cleanup();
	}

	/* Register client after loading graph so the to-ui ring does not overflow.
	   Since we are not yet rolling, it won't be drained, causing a deadlock. */
	std::shared_ptr<Interface> client(&driver->writer(), NullDeleter<Interface>);
	interface->set_respondee(client);
	engine->register_client(client);

	driver->set_instantiated(true);
	return static_cast<LV2_Handle>(plugin);
}

static void
ingen_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	auto*      me     = static_cast<IngenPlugin*>(instance);
	Engine*    engine = static_cast<Engine*>(me->world->engine().get());
	const auto driver = std::static_pointer_cast<LV2Driver>(engine->driver());
	if (port < driver->ports().size()) {
		driver->ports().at(port)->set_buffer(data);
	} else {
		engine->log().rt_error("Connect to non-existent port\n");
	}
}

static void
ingen_activate(LV2_Handle instance)
{
	auto*      me     = static_cast<IngenPlugin*>(instance);
	auto       engine = std::static_pointer_cast<Engine>(me->world->engine());
	const auto driver = std::static_pointer_cast<LV2Driver>(engine->driver());
	engine->activate();
	me->main = make_unique<std::thread>(ingen_lv2_main, engine, driver);
}

static void
ingen_run(LV2_Handle instance, uint32_t sample_count)
{
	auto*      me     = static_cast<IngenPlugin*>(instance);
	auto       engine = std::static_pointer_cast<Engine>(me->world->engine());
	const auto driver = std::static_pointer_cast<LV2Driver>(engine->driver());

	ThreadManager::set_flag(THREAD_PROCESS);
	ThreadManager::set_flag(THREAD_IS_REAL_TIME);

	driver->run(sample_count);
}

static void
ingen_deactivate(LV2_Handle instance)
{
	auto* me = static_cast<IngenPlugin*>(instance);
	me->world->engine()->deactivate();
	if (me->main) {
		me->main->join();
		me->main.reset();
	}
}

static void
ingen_cleanup(LV2_Handle instance)
{
	auto* me = static_cast<IngenPlugin*>(instance);
	me->world->set_engine(nullptr);
	me->world->set_interface(nullptr);
	if (me->main) {
		me->main->join();
		me->main.reset();
	}

	auto world = std::move(me->world);
	delete me;
}

static void
get_state_features(const LV2_Feature* const* features,
                   LV2_State_Map_Path**      map,
                   LV2_State_Make_Path**     make)
{
	for (int i = 0; features[i]; ++i) {
		if (map && !strcmp(features[i]->URI, LV2_STATE__mapPath)) {
			*map = static_cast<LV2_State_Map_Path*>(features[i]->data);
		} else if (make && !strcmp(features[i]->URI, LV2_STATE__makePath)) {
			*make = static_cast<LV2_State_Make_Path*>(features[i]->data);
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
	auto* plugin = static_cast<IngenPlugin*>(instance);

	LV2_State_Map_Path*  map_path  = nullptr;
	LV2_State_Make_Path* make_path = nullptr;
	get_state_features(features, &map_path, &make_path);
	if (!map_path || !make_path || !plugin->map) {
		plugin->world->log().error("Missing state:mapPath, state:makePath, or urid:Map\n");
		return LV2_STATE_ERR_NO_FEATURE;
	}

	LV2_URID ingen_file = plugin->map->map(plugin->map->handle, INGEN__file);
	LV2_URID atom_Path = plugin->map->map(plugin->map->handle,
	                                      LV2_ATOM__Path);

	char* real_path  = make_path->path(make_path->handle, "main.ttl");
	char* state_path = map_path->abstract_path(map_path->handle, real_path);

	auto root = plugin->world->store()->find(raul::Path("/"));

	{
		std::lock_guard<std::mutex> lock(plugin->world->rdf_mutex());

		plugin->world->serialiser()->start_to_file(
			root->second->path(), FilePath{real_path});
		plugin->world->serialiser()->serialise(root->second);
		plugin->world->serialiser()->finish();
	}

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
	auto* plugin = static_cast<IngenPlugin*>(instance);

	LV2_State_Map_Path* map_path = nullptr;
	get_state_features(features, &map_path, nullptr);
	if (!map_path) {
		plugin->world->log().error("Missing state:mapPath\n");
		return LV2_STATE_ERR_NO_FEATURE;
	}

	LV2_URID ingen_file = plugin->map->map(plugin->map->handle, INGEN__file);
	size_t   size       = 0;
	uint32_t type       = 0;
	uint32_t valflags   = 0;

	// Get abstract path to graph file
	const char* path = static_cast<const char*>(
		retrieve(handle, ingen_file, &size, &type, &valflags));
	if (!path) {
		return LV2_STATE_ERR_NO_PROPERTY;
	}

	// Convert to absolute path
	char* real_path = map_path->absolute_path(map_path->handle, path);
	if (!real_path) {
		return LV2_STATE_ERR_UNKNOWN;
	}

#if 0
	// Remove existing root graph contents
	std::shared_ptr<Engine> engine = plugin->engine;
	for (const auto& b : engine->root_graph()->blocks()) {
		plugin->world->interface()->del(b.uri());
	}

	const uint32_t n_ports = engine->root_graph()->num_ports_non_rt();
	for (int32_t i = n_ports - 1; i >= 0; --i) {
		PortImpl* port = engine->root_graph()->port_impl(i);
		if (port->symbol() != "control" && port->symbol() != "notify") {
			plugin->world->interface()->del(port->uri());
		}
	}
#endif

	// Load new graph
	std::lock_guard<std::mutex> lock(plugin->world->rdf_mutex());
	plugin->world->parser()->parse_file(
		*plugin->world, *plugin->world->interface(), real_path);

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
	return nullptr;
}

LV2Graph::LV2Graph(Parser::ResourceRecord record)
	: Parser::ResourceRecord(std::move(record))
	, descriptor()
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
	ingen::set_bundle_path(bundle_path);
	const std::string manifest_path = ingen::bundle_file_path("manifest.ttl");
	SerdNode          manifest_node =
	    serd_node_new_file_uri(reinterpret_cast<const uint8_t*>(
	                               manifest_path.c_str()),
	                           nullptr,
	                           nullptr,
	                           true);

	graphs = find_graphs(URI(reinterpret_cast<const char*>(manifest_node.buf)));

	serd_node_free(&manifest_node);
}

static void
lib_cleanup(LV2_Lib_Handle handle)
{
	Lib* lib = static_cast<Lib*>(handle);
	delete lib;
}

static const LV2_Descriptor*
lib_get_plugin(LV2_Lib_Handle handle, uint32_t index)
{
	Lib* lib = static_cast<Lib*>(handle);
	return index < lib->graphs.size() ? &lib->graphs[index]->descriptor : nullptr;
}

} // extern "C"
} // namespace server
} // namespace ingen

extern "C" {

/** LV2 plugin library entry point */
LV2_SYMBOL_EXPORT
const LV2_Lib_Descriptor*
lv2_lib_descriptor(const char*              bundle_path,
                   const LV2_Feature*const* features)
{
	static const uint32_t desc_size = sizeof(LV2_Lib_Descriptor);
	auto*                 lib       = new ingen::server::Lib(bundle_path);

	// FIXME: memory leak.  I think the LV2_Lib_Descriptor API is botched :(
	auto* desc = static_cast<LV2_Lib_Descriptor*>(malloc(desc_size));
	desc->handle     = lib;
	desc->size       = desc_size;
	desc->cleanup    = ingen::server::lib_cleanup;
	desc->get_plugin = ingen::server::lib_get_plugin;

	return desc;
}

}
