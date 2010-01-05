/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef ENGINE_H
#define ENGINE_H

#include <cassert>
#include <vector>
#include <set>
#include <boost/utility.hpp>
#include "raul/SharedPtr.hpp"
#include "module/World.hpp"
#include "interface/PortType.hpp"
#include "interface/EventType.hpp"

template<typename T> class Queue;

namespace Raul { class Maid; }

namespace Ingen {

class Driver;
class BufferFactory;
class ClientBroadcaster;
class Driver;
class EngineStore;
class Event;
class EventSource;
class MessageContext;
class NodeFactory;
class PostProcessor;
class ProcessContext;
class ProcessSlave;
class QueuedEngineInterface;
class QueuedEvent;


/** The main class for the Engine.
 *
 * This is a (GoF) facade for the engine.  Pointers to all components are
 * available for more advanced control than this facade allows.
 *
 * Most objects in the engine have (directly or indirectly) a pointer to the
 * Engine they are a part of.
 *
 * \ingroup engine
 */
class Engine : boost::noncopyable
{
public:
	Engine(Ingen::Shared::World* world);

	virtual ~Engine();

	virtual int  main();
	virtual bool main_iteration();

	/** Set the quit flag that should kill all threads and exit cleanly.
	 * Note that it will take some time. */
	virtual void quit() { _quit_flag = true; }

	virtual bool activate();
	virtual void deactivate();

	void process_events(ProcessContext& context);

	virtual bool activated() { return _activated; }

	Raul::Maid*        maid()            const { return _maid; }
	Driver*            driver()          const { return _driver.get(); }
	PostProcessor*     post_processor()  const { return _post_processor; }
	ClientBroadcaster* broadcaster()     const { return _broadcaster; }
	NodeFactory*       node_factory()    const { return _node_factory; }
	MessageContext*    message_context() const { return _message_context; }
	BufferFactory*     buffer_factory()  const { return _buffer_factory; }

	SharedPtr<EngineStore> engine_store() const;

	virtual void set_driver(SharedPtr<Driver> driver) { _driver = driver; }

	virtual void add_event_source(SharedPtr<EventSource> source);

	Ingen::QueuedEngineInterface* new_local_interface();

	Ingen::Shared::World* world() { return _world; }

	typedef std::vector<ProcessSlave*> ProcessSlaves;
	inline const ProcessSlaves& process_slaves() const { return _process_slaves; }
	inline ProcessSlaves& process_slaves() { return _process_slaves; }

private:
	typedef std::set< SharedPtr<EventSource> > EventSources;
	EventSources _event_sources;

	ProcessSlaves          _process_slaves;
	Ingen::Shared::World*  _world;
	SharedPtr<Driver>      _driver;
	SharedPtr<Driver>      _midi_driver;
	Raul::Maid*            _maid;
	PostProcessor*         _post_processor;
	ClientBroadcaster*     _broadcaster;
	NodeFactory*           _node_factory;
	MessageContext*        _message_context;
	BufferFactory*         _buffer_factory;

	bool _quit_flag;
	bool _activated;
};


} // namespace Ingen

#endif // ENGINE_H
