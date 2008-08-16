/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include CONFIG_H_PATH
#include <cassert>
#include <vector>
#include <boost/utility.hpp>
#include <raul/SharedPtr.hpp>
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
class QueuedEngineInterface;
class Driver;
class ProcessSlave;
class ProcessContext;


/** The main class for the Engine.
 *
 * This is a (GoF) facade for the engine.  Pointers to all components are
 * available for more advanced control than this facade allows.
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

	virtual void start_jack_driver();
	virtual void start_osc_driver(int port);
	virtual void start_http_driver(int port);
	
	virtual SharedPtr<QueuedEngineInterface> new_queued_interface();

	virtual bool activate(size_t parallelism);
	virtual void deactivate();

	void process_events(ProcessContext& context);

	virtual bool activated() { return _activated; }

	Raul::Maid*        maid()               const { return _maid; }
	EventSource*       event_source()       const { return _event_source.get(); }
	AudioDriver*       audio_driver()       const { return _audio_driver.get(); }
	MidiDriver*        midi_driver()        const { return _midi_driver; }
	OSCDriver*         osc_driver()         const { return _osc_driver; }
	PostProcessor*     post_processor()     const { return _post_processor; }
	ClientBroadcaster* broadcaster()        const { return _broadcaster; }
	NodeFactory*       node_factory()       const { return _node_factory; }

	SharedPtr<EngineStore> engine_store() const;

	/** Return the active driver for the given type */
	Driver* driver(DataType type, EventType event_type);

	Ingen::Shared::World* world() { return _world; }

	typedef std::vector<ProcessSlave*> ProcessSlaves;
	inline const ProcessSlaves& process_slaves() const { return _process_slaves; }
	inline ProcessSlaves& process_slaves() { return _process_slaves; }
	
private:
	ProcessSlaves _process_slaves;

	Ingen::Shared::World* _world;

	SharedPtr<EventSource> _event_source;
	SharedPtr<AudioDriver> _audio_driver;
	MidiDriver*            _midi_driver;
	OSCDriver*             _osc_driver;
	Raul::Maid*            _maid;
	PostProcessor*         _post_processor;
	ClientBroadcaster*     _broadcaster;
	NodeFactory*           _node_factory;
	
	bool _quit_flag;
	bool _activated;
};


} // namespace Ingen

#endif // ENGINE_H
