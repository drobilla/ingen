/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

/**
   The engine which executes the process graph.

   This is a simple class that provides pointers to the various components
   that make up the engine implementation.  In processes with a local engine,
   it can be accessed via the Ingen::Shared::World.

   @ingroup engine
*/
class Engine : public boost::noncopyable
{
public:
	explicit Engine(Ingen::Shared::World* world);

	virtual ~Engine();

	virtual void set_driver(SharedPtr<Driver> driver);
	virtual void add_event_source(SharedPtr<EventSource> source);

	virtual bool activate();
	virtual void deactivate();

	/**
	   Indicate that a quit is desired

	   This function simply sets a flag which affects the return value of
	   main_iteration, it does not actually force the engine to stop running or
	   block.  The code driving the engine is responsible for stopping and
	   cleaning up when main_iteration returns false.
	*/
	virtual void quit();

	/**
	   Run a single iteration of the main context.

	   The main context post-processes events and performs housekeeping duties
	   like collecting garbage.  This should be called regularly, e.g. a few
	   times per second.  The return value indicates whether execution should
	   continue; i.e. if false is returned, a quit has been requested and the
	   caller should cease calling main_iteration() and stop the engine.
	*/
	virtual bool main_iteration();

	/**
	   Process all events for this process cycle.

	   Must be called (likely by the Driver) from the process thread.
	*/
	virtual void process_events(ProcessContext& context);

	virtual ClientBroadcaster* broadcaster()      const { return _broadcaster; }
	virtual BufferFactory*     buffer_factory()   const { return _buffer_factory; }
	virtual ControlBindings*   control_bindings() const { return _control_bindings; }
	virtual Driver*            driver()           const { return _driver.get(); }
	virtual Raul::Maid*        maid()             const { return _maid; }
	virtual MessageContext*    message_context()  const { return _message_context; }
	virtual NodeFactory*       node_factory()     const { return _node_factory; }
	virtual PostProcessor*     post_processor()   const { return _post_processor; }
	virtual Shared::World*     world()            const { return _world; }

	virtual SharedPtr<EngineStore> engine_store() const;

private:
	typedef std::set< SharedPtr<EventSource> > EventSources;
	EventSources _event_sources;

	ClientBroadcaster*    _broadcaster;
	BufferFactory*        _buffer_factory;
	ControlBindings*      _control_bindings;
	SharedPtr<Driver>     _driver;
	Raul::Maid*           _maid;
	MessageContext*       _message_context;
	NodeFactory*          _node_factory;
	PostProcessor*        _post_processor;
	Shared::World*        _world;

	bool _quit_flag;
};

} // namespace Ingen

#endif // INGEN_ENGINE_ENGINE_HPP
