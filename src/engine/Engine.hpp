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
#include "module/global.hpp"
#include "interface/DataType.hpp"
#include "interface/EventType.hpp"

template<typename T> class Queue;

namespace Raul { class Maid; }

namespace Ingen {

class AudioDriver;
class MidiDriver;
class OSCDriver;
class NodeFactory;
class ClientBroadcaster;
class EngineStore;
class EventSource;
class PostProcessor;
class Event;
class QueuedEvent;
class Driver;
class ProcessSlave;
class ProcessContext;
class MessageContext;


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

	virtual bool activate(size_t parallelism);
	virtual void deactivate();

	void process_events(ProcessContext& context);

	virtual bool activated() { return _activated; }

	Raul::Maid*        maid()               const { return _maid; }
	AudioDriver*       audio_driver()       const { return _audio_driver.get(); }
	MidiDriver*        midi_driver()        const { return _midi_driver; }
	OSCDriver*         osc_driver()         const { return _osc_driver; }
	PostProcessor*     post_processor()     const { return _post_processor; }
	ClientBroadcaster* broadcaster()        const { return _broadcaster; }
	NodeFactory*       node_factory()       const { return _node_factory; }
	MessageContext*    message_context()    const { return _message_context; }

	SharedPtr<EngineStore> engine_store() const;

	/** Return the active driver for the given type */
	Driver* driver(Shared::DataType type, Shared::EventType event_type);

	/** Set the driver for the given data type (replacing the old) */
	virtual void set_driver(Shared::DataType type, SharedPtr<Driver> driver);
	virtual void set_midi_driver(MidiDriver* driver);

	virtual void add_event_source(SharedPtr<EventSource> source);

	Ingen::Shared::World* world() { return _world; }

	typedef std::vector<ProcessSlave*> ProcessSlaves;
	inline const ProcessSlaves& process_slaves() const { return _process_slaves; }
	inline ProcessSlaves& process_slaves() { return _process_slaves; }

private:
	typedef std::set< SharedPtr<EventSource> > EventSources;
	EventSources _event_sources;

	ProcessSlaves          _process_slaves;
	Ingen::Shared::World*  _world;
	SharedPtr<AudioDriver> _audio_driver;
	MidiDriver*            _midi_driver;
	OSCDriver*             _osc_driver;
	Raul::Maid*            _maid;
	PostProcessor*         _post_processor;
	ClientBroadcaster*     _broadcaster;
	NodeFactory*           _node_factory;
	MessageContext*        _message_context;

	bool _quit_flag;
	bool _activated;
};


} // namespace Ingen

#endif // ENGINE_H
