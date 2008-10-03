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

#include <cassert>
#include <sys/mman.h>
#include <iostream>
#include <unistd.h>
#include <raul/Deletable.hpp>
#include <raul/Maid.hpp>
#include <raul/SharedPtr.hpp>
#include "Engine.hpp"	
#include "config.h"
#include "tuning.hpp"
#include "Event.hpp"
#include "common/interface/EventType.hpp"
#include "shared/Store.hpp"
#include "JackAudioDriver.hpp"
#include "NodeFactory.hpp"
#include "ClientBroadcaster.hpp"
#include "PatchImpl.hpp"
#include "EngineStore.hpp"
#include "MidiDriver.hpp"
#include "OSCDriver.hpp"
#include "QueuedEventSource.hpp"
#include "PostProcessor.hpp"
#include "events/CreatePatchEvent.hpp"
#include "events/EnablePatchEvent.hpp"
#include "OSCEngineReceiver.hpp"
#ifdef HAVE_SOUP
#include "HTTPEngineReceiver.hpp"
#endif
#include "PostProcessor.hpp"
#include "ProcessSlave.hpp"
#include "ProcessContext.hpp"
#include "MessageContext.hpp"
#include "ThreadManager.hpp"
#ifdef HAVE_JACK_MIDI
#include "JackMidiDriver.hpp"
#endif
using namespace std;

namespace Ingen {


Engine::Engine(Ingen::Shared::World* world)
  	: _world(world)
	, _midi_driver(NULL)
	, _osc_driver(NULL)
	, _maid(new Raul::Maid(maid_queue_size))
	, _post_processor(new PostProcessor(*this, /**_maid, */post_processor_queue_size))
	, _broadcaster(new ClientBroadcaster())
	, _node_factory(new NodeFactory(world))
	, _message_context(new MessageContext(*this))
	, _quit_flag(false)
	, _activated(false)
{
	if (world->store) {
		assert(PtrCast<EngineStore>(world->store));
	} else {
		world->store = SharedPtr<Store>(new EngineStore());
	}
}


Engine::~Engine()
{
	deactivate();

	for (EngineStore::iterator i = engine_store()->begin();
			i != engine_store()->end(); ++i) {
		if ( ! PtrCast<GraphObjectImpl>(i->second)->parent() )
			i->second.reset();
	}

	delete _broadcaster;
	delete _node_factory;
	delete _osc_driver;
	delete _post_processor;
	//delete _lash_driver;

	delete _maid;
	
	munlockall();
}
	

SharedPtr<EngineStore>
Engine::engine_store() const
{
	 return PtrCast<EngineStore>(_world->store);
}


Driver*
Engine::driver(DataType type, EventType event_type)
{
	if (type == DataType::AUDIO) {
		return _audio_driver.get();
	} else if (type == DataType::EVENT) {
		if (event_type == EventType::MIDI) {
			return _midi_driver;
		} else if (event_type == EventType::OSC) {
			return _osc_driver;
		}
	}

	return NULL;
}


int
Engine::main()
{
	Thread::get().set_context(THREAD_POST_PROCESS);

	// Loop until quit flag is set (by OSCReceiver)
	while ( ! _quit_flag) {
		nanosleep(&main_rate, NULL);
		main_iteration();
	}
	cout << "[Main] Done main loop." << endl;
	
	deactivate();

	return 0;
}


/** Run one iteration of the main loop.
 *
 * NOT realtime safe (this is where deletion actually occurs)
 */
bool
Engine::main_iteration()
{
/*#ifdef HAVE_LASH
	// Process any pending LASH events
	if (lash_driver->enabled())
		lash_driver->process_events();
#endif*/
	// Run the maid (garbage collector)
    _post_processor->process();
	_maid->cleanup();
	
	return !_quit_flag;
}


void
Engine::start_jack_driver()
{
	if ( ! _audio_driver)
		_audio_driver = SharedPtr<AudioDriver>(new JackAudioDriver(*this));
	else
		cerr << "[Engine::start_jack_driver] Audio driver already running" << endl;
}


void
Engine::start_osc_driver(int port)
{
	if (_event_source) {
		cerr << "WARNING: Replacing event source" << endl;
		_event_source.reset();
	}

	_event_source = SharedPtr<EventSource>(new OSCEngineReceiver(
			*this, pre_processor_queue_size, port));
}

	
void
Engine::start_http_driver(int port)
{
#ifdef HAVE_SOUP
	// FIXE: leak
	HTTPEngineReceiver* server = new HTTPEngineReceiver(*this, port);
	server->activate();
#endif
}
	

SharedPtr<QueuedEngineInterface>
Engine::new_queued_interface()
{
	if (_event_source) {
		cerr << "WARNING: Replacing event source" << endl;
		_event_source.reset();
	}

	SharedPtr<QueuedEngineInterface> result(new QueuedEngineInterface(
			*this, Ingen::event_queue_size, Ingen::event_queue_size));
	
	_event_source = result;

	return result;
}

/*
void
Engine::set_event_source(SharedPtr<EventSource> source)
{
	if (_event_source)
		cerr << "Warning:  Dropped event source (engine interface)" << endl;

	_event_source = source;
}
*/


bool
Engine::activate(size_t parallelism)
{
	if (_activated)
		return false;

	assert(_audio_driver);
	assert(_event_source);

#ifdef HAVE_JACK_MIDI
	_midi_driver = new JackMidiDriver(((JackAudioDriver*)_audio_driver.get())->jack_client());
#else
	_midi_driver = new DummyMidiDriver();
#endif
	
	_event_source->activate();

	// Create root patch

	PatchImpl* root_patch = new PatchImpl(*this, "", 1, NULL,
			_audio_driver->sample_rate(), _audio_driver->buffer_size(), 1);
	root_patch->activate();
	_world->store->add(root_patch);
	root_patch->compiled_patch(root_patch->compile());

	assert(_audio_driver->root_patch() == NULL);
	_audio_driver->set_root_patch(root_patch);

	_audio_driver->activate();

	_process_slaves.clear();
	_process_slaves.reserve(parallelism);
	for (size_t i=0; i < parallelism - 1; ++i)
		_process_slaves.push_back(new ProcessSlave(*this, _audio_driver->is_realtime()));
	
	root_patch->enable();
	
	//_post_processor->start();

	_activated = true;
	
	return true;
}


void
Engine::deactivate()
{
	if (!_activated)
		return;
	
	_event_source->deactivate();

	/*for (Tree<GraphObject*>::iterator i = _engine_store->objects().begin();
			i != _engine_store->objects().end(); ++i)
		if ((*i)->as_node() != NULL && (*i)->as_node()->parent() == NULL)
			(*i)->as_node()->deactivate();*/
	
	if (_midi_driver != NULL) {
		_midi_driver->deactivate();
		delete _midi_driver;
		_midi_driver = NULL;
	}
	
	_audio_driver->deactivate();

	_audio_driver->root_patch()->deactivate();
	
	for (size_t i=0; i < _process_slaves.size(); ++i) {
		delete _process_slaves[i];
	}
	
	_process_slaves.clear();

	// Finalize any lingering events (unlikely)
	_post_processor->process();

	_audio_driver.reset();
	_event_source.reset();
	
	_activated = false;
}
	

void
Engine::process_events(ProcessContext& context)
{
	if (_event_source)
		_event_source->process(*_post_processor, context);
}


} // namespace Ingen