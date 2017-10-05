/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_ENGINE_HPP
#define INGEN_ENGINE_ENGINE_HPP

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <random>

#include "ingen/Clock.hpp"
#include "ingen/EngineBase.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Properties.hpp"
#include "ingen/ingen.h"
#include "ingen/types.hpp"
#include "raul/Noncopyable.hpp"

#include "Event.hpp"
#include "EventWriter.hpp"
#include "Load.hpp"
#include "RunContext.hpp"

namespace Raul { class Maid; }

namespace Ingen {

class AtomReader;
class Store;
class World;

namespace Server {

class BlockFactory;
class Broadcaster;
class BufferFactory;
class ControlBindings;
class Driver;
class GraphImpl;
class LV2Options;
class PostProcessor;
class PreProcessor;
class RunContext;
class SocketListener;
class UndoStack;
class Worker;

/**
   The engine which executes the process graph.

   This is a simple class that provides pointers to the various components
   that make up the engine implementation.  In processes with a local engine,
   it can be accessed via the Ingen::World.

   @ingroup engine
*/
class INGEN_API Engine : public Raul::Noncopyable, public EngineBase
{
public:
	explicit Engine(Ingen::World* world);

	virtual ~Engine();

	// EngineBase methods
	virtual void init(double sample_rate, uint32_t block_length, size_t seq_size);
	virtual bool supports_dynamic_ports() const;
	virtual bool activate();
	virtual void deactivate();
	virtual bool pending_events() const;
	virtual unsigned run(uint32_t sample_count);
	virtual void quit();
	virtual bool main_iteration();
	virtual void register_client(SPtr<Interface> client);
	virtual bool unregister_client(SPtr<Interface> client);

	void listen();

	/** Return a random [0..1] float with uniform distribution */
	float frand() { return _uniform_dist(_rand_engine); }

	void set_driver(SPtr<Driver> driver);

	/** Return the frame time to execute an event that arrived now.
	 *
	 * This aims to return a time one cycle from "now", so that events ideally
	 * have 1 cycle of latency with no jitter.
	 */
	SampleCount event_time();

	/** Return the time this cycle began processing in microseconds.
	 *
	 * This value is comparable to the value returned by current_time().
	 */
	inline uint64_t cycle_start_time(const RunContext& context) const {
		return _cycle_start_time;
	}

	/** Return the current time in microseconds. */
	uint64_t current_time() const;

	/** Reset the load statistics (when the expected DSP load changes). */
	void reset_load();

	/** Enqueue an event to be processed (non-realtime threads only). */
	void enqueue_event(Event* ev, Event::Mode mode=Event::Mode::NORMAL);

	/** Process events (process thread only). */
	unsigned process_events();

	/** Process all events (no RT limits). */
	unsigned process_all_events();

	Ingen::World* world() const { return _world; }

	Interface*       interface()        const { return _interface.get(); }
	EventWriter*     event_writer()     const { return _event_writer.get(); }
	AtomReader*      atom_interface()   const { return _atom_interface; }
	BlockFactory*    block_factory()    const { return _block_factory; }
	Broadcaster*     broadcaster()      const { return _broadcaster; }
	BufferFactory*   buffer_factory()   const { return _buffer_factory; }
	ControlBindings* control_bindings() const { return _control_bindings; }
	Driver*          driver()           const { return _driver.get(); }
	Log&             log()              const { return _world->log(); }
	GraphImpl*       root_graph()       const { return _root_graph; }
	PostProcessor*   post_processor()   const { return _post_processor; }
	Raul::Maid*      maid()             const { return _maid; }
	UndoStack*       undo_stack()       const { return _undo_stack; }
	UndoStack*       redo_stack()       const { return _redo_stack; }
	Worker*          worker()           const { return _worker; }
	Worker*          sync_worker()      const { return _sync_worker; }

	RunContext& run_context() { return *_run_contexts[0]; }

	void set_root_graph(GraphImpl* graph) { _root_graph = graph; }

	void flush_events(const std::chrono::milliseconds& sleep_ms);

	void  advance(SampleCount nframes);
	void  locate(FrameTime s, SampleCount nframes);
	void  emit_notifications(FrameTime end);
	bool  pending_notifications();
	bool  wait_for_tasks();
	void  signal_tasks_available();
	Task* steal_task(unsigned start_thread);

	SPtr<Store> store() const;

	SampleRate  sample_rate() const;
	SampleCount block_length() const;
	size_t      sequence_size() const;
	size_t      event_queue_size() const;

	size_t n_threads()      const { return _run_contexts.size(); }
	bool   atomic_bundles() const { return _atomic_bundles; }
	bool   activated()      const { return _activated; }

	Properties load_properties() const;

private:
	Ingen::World* _world;

	Raul::Maid*       _maid;
	BlockFactory*     _block_factory;
	Broadcaster*      _broadcaster;
	BufferFactory*    _buffer_factory;
	ControlBindings*  _control_bindings;
	SPtr<Driver>      _driver;
	SPtr<EventWriter> _event_writer;
	SPtr<Interface>   _interface;
	AtomReader*       _atom_interface;
	SPtr<LV2Options>  _options;
	UndoStack*        _undo_stack;
	UndoStack*        _redo_stack;
	PreProcessor*     _pre_processor;
	PostProcessor*    _post_processor;
	GraphImpl*        _root_graph;
	Worker*           _worker;
	Worker*           _sync_worker;
	SocketListener*   _listener;

	std::vector<Raul::RingBuffer*> _notifications;
	std::vector<RunContext*>       _run_contexts;
	uint64_t                       _cycle_start_time;
	Load                           _run_load;
	Clock                          _clock;

	std::mt19937                          _rand_engine;
	std::uniform_real_distribution<float> _uniform_dist;

	std::condition_variable _tasks_available;
	std::mutex              _tasks_mutex;

	bool _quit_flag;
	bool _reset_load_flag;
	bool _atomic_bundles;
	bool _activated;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_ENGINE_HPP
