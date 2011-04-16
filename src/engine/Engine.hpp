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

#ifndef INGEN_ENGINE_ENGINE_HPP
#define INGEN_ENGINE_ENGINE_HPP

#include <set>
#include <vector>

#include <boost/utility.hpp>

#include "raul/SharedPtr.hpp"

#include "ingen/PortType.hpp"
#include "ingen/EventType.hpp"

namespace Raul { class Maid; }

namespace Ingen {

namespace Shared { class World; }

class BufferFactory;
class ClientBroadcaster;
class ControlBindings;
class Driver;
class EngineStore;
class EventSource;
class MessageContext;
class NodeFactory;
class PostProcessor;
class ProcessContext;
class ProcessSlave;


/**
   The engine which executes the process graph.

   This is a simple class that provides pointers to the various components
   that make up the engine implementation.  In processes with a local engine,
   it can be accessed via the Ingen::Shared::World.

  \ingroup engine
*/
class Engine : public boost::noncopyable
{
public:
	explicit Engine(Ingen::Shared::World* world);

	virtual ~Engine();

	virtual bool activate();
	virtual void deactivate();

	/**
	   Indicate that a quit is desired

	   This function simply sets a flag which affects the return value of
	   main iteration, it does not actually force the engine to stop running
	   or block.  The code driving the engine is responsible for stopping
	   and cleaning up when main_iteration returns false.
	*/
	virtual void quit();

	/**
	   Run a single iteration of the main context.

	   The main context performs housekeeping duties like collecting garbage.
	   This should be called regularly, e.g. a few times per second.
	   The return value indicates whether execution should continue; i.e. if
	   false is returned, the caller should cease calling main_iteration()
	   and stop the engine.
	*/
	virtual bool main_iteration();

	virtual void process_events(ProcessContext& context);

	virtual bool activated();

	virtual BufferFactory*     buffer_factory()   const { return _buffer_factory; }
	virtual ClientBroadcaster* broadcaster()      const { return _broadcaster; }
	virtual ControlBindings*   control_bindings() const { return _control_bindings; }
	virtual Driver*            driver()           const { return _driver.get(); }
	virtual MessageContext*    message_context()  const { return _message_context; }
	virtual NodeFactory*       node_factory()     const { return _node_factory; }
	virtual PostProcessor*     post_processor()   const { return _post_processor; }
	virtual Raul::Maid*        maid()             const { return _maid; }

	virtual SharedPtr<EngineStore> engine_store() const;

	virtual void set_driver(SharedPtr<Driver> driver) { _driver = driver; }

	virtual void add_event_source(SharedPtr<EventSource> source);

	virtual Ingen::Shared::World* world() { return _world; }

	typedef std::vector<ProcessSlave*> ProcessSlaves;
	virtual const ProcessSlaves& process_slaves() const { return _process_slaves; }
	virtual ProcessSlaves&       process_slaves()       { return _process_slaves; }

private:
	typedef std::set< SharedPtr<EventSource> > EventSources;
	EventSources _event_sources;

	ProcessSlaves         _process_slaves;
	Ingen::Shared::World* _world;
	SharedPtr<Driver>     _driver;
	Raul::Maid*           _maid;
	PostProcessor*        _post_processor;
	ClientBroadcaster*    _broadcaster;
	NodeFactory*          _node_factory;
	MessageContext*       _message_context;
	BufferFactory*        _buffer_factory;
	ControlBindings*      _control_bindings;

	bool _quit_flag;
	bool _activated;
};


} // namespace Ingen

#endif // INGEN_ENGINE_ENGINE_HPP
