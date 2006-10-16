/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

template<typename T> class Queue;
class Maid;

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
template <typename T> class Driver;


/** The main class for the Engine.
 *
 * With access to this you can find any object that's a part of the engine.
 * Access to this should be limited as possible, it basically exists so
 * there's something to pass Event constructors so they can access what
 * they need to perform their function.
 *
 * \ingroup engine
 */
class Engine : boost::noncopyable
{
public:
	Engine(AudioDriver* audio_driver = 0);
	~Engine();

	int  main();
	bool main_iteration();
	
	/** Set the quit flag that should kill all threads and exit cleanly.
	 * Note that it will take some time. */
	void quit() { m_quit_flag = true; }
	
	void activate();
	void deactivate();

	bool activated() { return m_activated; }

	void set_event_source(EventSource* es) { m_event_source = es; }

	EventSource*       event_source()       const { return m_event_source; }
	AudioDriver*       audio_driver()       const { return m_audio_driver; }
	MidiDriver*        midi_driver()        const { return m_midi_driver; }
	Maid*              maid()               const { return m_maid; }
	PostProcessor*     post_processor()     const { return m_post_processor; }
	ClientBroadcaster* broadcaster()        const { return m_broadcaster; }
	ObjectStore*       object_store()       const { return m_object_store; }
	NodeFactory*       node_factory()       const { return m_node_factory; }
	LashDriver*        lash_driver()        const { return m_lash_driver; }

	/** Return the active driver for the given (template parameter) type */
	template<typename T> Driver<T>* driver();
	
private:
	EventSource*       m_event_source;
	AudioDriver*       m_audio_driver;
	MidiDriver*        m_midi_driver;
	Maid*              m_maid;
	PostProcessor*     m_post_processor;
	ClientBroadcaster* m_broadcaster;
	ObjectStore*       m_object_store;
	NodeFactory*       m_node_factory;
	LashDriver*        m_lash_driver;
	
	bool m_quit_flag;
	bool m_activated;
};


} // namespace Ingen

#endif // ENGINE_H
