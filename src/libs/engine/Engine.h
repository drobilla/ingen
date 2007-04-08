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

#include <cassert>
#include <boost/utility.hpp>
#include <raul/SharedPtr.h>
#include "DataType.h"

template<typename T> class Queue;

namespace Raul { class Maid; }

namespace Ingen {

class AudioDriver;
class MidiDriver;
class NodeFactory;
class ClientBroadcaster;
class Patch;
class ObjectStore;
class EventSource;
class PostProcessor;
class Event;
class QueuedEvent;
class LashDriver;
class Driver;


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
	Engine();
	~Engine();

	int  main();
	bool main_iteration();
	
	/** Set the quit flag that should kill all threads and exit cleanly.
	 * Note that it will take some time. */
	void quit() { _quit_flag = true; }
	
	bool activate(SharedPtr<AudioDriver> ad, SharedPtr<EventSource> es);
	void deactivate();

	bool activated() { return _activated; }

	Raul::Maid*        maid()               const { return _maid; }
	EventSource*       event_source()       const { return _event_source.get(); }
	AudioDriver*       audio_driver()       const { return _audio_driver.get(); }
	MidiDriver*        midi_driver()        const { return _midi_driver; }
	PostProcessor*     post_processor()     const { return _post_processor; }
	ClientBroadcaster* broadcaster()        const { return _broadcaster; }
	ObjectStore*       object_store()       const { return _object_store; }
	NodeFactory*       node_factory()       const { return _node_factory; }
	LashDriver*        lash_driver()        const { return _lash_driver; }

	/** Return the active driver for the given type */
	Driver* driver(DataType type);
	
private:
	SharedPtr<EventSource> _event_source;
	SharedPtr<AudioDriver> _audio_driver;
	MidiDriver*            _midi_driver;
	Raul::Maid*            _maid;
	PostProcessor*         _post_processor;
	ClientBroadcaster*     _broadcaster;
	ObjectStore*           _object_store;
	NodeFactory*           _node_factory;
	LashDriver*            _lash_driver;
	
	bool _quit_flag;
	bool _activated;
};


} // namespace Ingen

#endif // ENGINE_H
