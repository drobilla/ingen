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

#ifndef INGEN_ENGINE_ENGINE_HPP
#define INGEN_ENGINE_ENGINE_HPP

#include <random>

#include <boost/utility.hpp>

#include "ingen/EngineBase.hpp"
#include "ingen/Interface.hpp"
#include "ingen/ingen.h"
#include "ingen/types.hpp"

#include "Event.hpp"
#include "ProcessContext.hpp"

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
class EventWriter;
class GraphImpl;
class LV2Options;
class PostProcessor;
class PreProcessor;
class ProcessContext;
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
class INGEN_API Engine : public boost::noncopyable, public EngineBase
{
public:
	explicit Engine(Ingen::World* world);

	virtual ~Engine();

	// EngineBase methods
	virtual void init(double sample_rate, uint32_t block_length, size_t seq_size);
	virtual bool activate();
	virtual void deactivate();
	virtual bool pending_events();
	virtual unsigned run(uint32_t sample_count);
	virtual void quit();
	virtual bool main_iteration();
	virtual void register_client(SPtr<Interface> client);
	virtual bool unregister_client(SPtr<Interface> client);

	void listen();

	/** Return a random [0..1] float with uniform distribution */
	float frand() { return _uniform_dist(_rand_engine); }

	void set_driver(SPtr<Driver> driver);

	SampleCount event_time();

	/** Enqueue an event to be processed (non-realtime threads only). */
	void enqueue_event(Event* ev, Event::Mode mode=Event::Mode::NORMAL);

	/** Process events (process thread only). */
	unsigned process_events();

	bool is_process_context(const Context& context) const {
		return &context == &_process_context;
	}

	Ingen::World* world() const { return _world; }

	EventWriter*     interface()        const { return _event_writer; }
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

	ProcessContext& process_context() { return _process_context; }

	SPtr<Store> store() const;

	size_t event_queue_size() const;

private:
	Ingen::World* _world;

	BlockFactory*    _block_factory;
	Broadcaster*     _broadcaster;
	BufferFactory*   _buffer_factory;
	ControlBindings* _control_bindings;
	SPtr<Driver>     _driver;
	EventWriter*     _event_writer;
	AtomReader*      _atom_interface;
	Raul::Maid*      _maid;
	SPtr<LV2Options> _options;
	UndoStack*       _undo_stack;
	UndoStack*       _redo_stack;
	PreProcessor*    _pre_processor;
	PostProcessor*   _post_processor;
	GraphImpl*       _root_graph;
	Worker*          _worker;
	Worker*          _sync_worker;
	SocketListener*  _listener;

	ProcessContext _process_context;

	std::mt19937                          _rand_engine;
	std::uniform_real_distribution<float> _uniform_dist;

	bool _quit_flag;
	bool _direct_driver;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_ENGINE_HPP
