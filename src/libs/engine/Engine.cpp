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

#include <cassert>
#include "Engine.h"	
#include "config.h"
#include "tuning.h"
#include <sys/mman.h>
#include <iostream>
#include <unistd.h>
#include "Event.h"
#include "JackAudioDriver.h"
#include "NodeFactory.h"
#include "ClientBroadcaster.h"
#include "Patch.h"
#include "ObjectStore.h"
#include "MaidObject.h"
#include "Maid.h"
#include "MidiDriver.h"
#include "QueuedEventSource.h"
#include "PostProcessor.h"
#include "CreatePatchEvent.h"
#include "EnablePatchEvent.h"
#ifdef HAVE_JACK_MIDI
#include "JackMidiDriver.h"
#endif
#ifdef HAVE_ALSA_MIDI
#include "AlsaMidiDriver.h"
#endif
#ifdef HAVE_LASH
#include "LashDriver.h"
#endif
using std::cout; using std::cerr; using std::endl;

namespace Ingen {


Engine::Engine()
: _midi_driver(NULL),
  _maid(new Maid(maid_queue_size)),
  _post_processor(new PostProcessor(*_maid, post_processor_queue_size)),
  _broadcaster(new ClientBroadcaster()),
  _object_store(new ObjectStore()),
  _node_factory(new NodeFactory()),
#ifdef HAVE_LASH
  _lash_driver(new LashDriver()),
#else 
  _lash_driver(NULL),
#endif
  _quit_flag(false),
  _activated(false)
{
}


Engine::~Engine()
{
	deactivate();

	for (Tree<GraphObject*>::iterator i = _object_store->objects().begin();
			i != _object_store->objects().end(); ++i) {
		if ((*i)->parent() == NULL)
			delete (*i);
	}
	
	delete _object_store;
	delete _broadcaster;
	delete _node_factory;
	delete _midi_driver;
	
	delete _maid;

	munlockall();
}


/* driver() template specializations.
 * Due to the lack of RTTI, this needs to be implemented manually like this.
 * If more types/drivers start getting added, it may be worth it to enable
 * RTTI and put all the drivers into a map with typeid's as the key.  That's
 * more elegant and extensible, but this is faster and simpler - for now.
 */
template<>
Driver<MidiMessage>* Engine::driver<MidiMessage>() { return _midi_driver; }
template<>
Driver<Sample>* Engine::driver<Sample>() { return _audio_driver.get(); }


int
Engine::main()
{
	// Loop until quit flag is set (by OSCReceiver)
	while ( ! _quit_flag) {
		nanosleep(&main_rate, NULL);
		main_iteration();
	}
	cout << "[Main] Done main loop." << endl;
	
	if (_activated)
		deactivate();

	sleep(1);
	cout << "[Main] Exiting..." << endl;
	
	return 0;
}


/** Run one iteration of the main loop.
 *
 * NOT realtime safe (this is where deletion actually occurs)
 */
bool
Engine::main_iteration()
{
#ifdef HAVE_LASH
	// Process any pending LASH events
	if (lash_driver->enabled())
		lash_driver->process_events();
#endif
	// Run the maid (garbage collector)
	_maid->cleanup();
	
	return !_quit_flag;
}


bool
Engine::activate(SharedPtr<AudioDriver> ad, SharedPtr<EventSource> es)
{
	if (_activated)
		return false;

	// Setup drivers
	_audio_driver = ad;
#ifdef HAVE_JACK_MIDI
	_midi_driver = new JackMidiDriver(((JackAudioDriver*)_audio_driver.get())->jack_client());
#elif HAVE_ALSA_MIDI
	_midi_driver = new AlsaMidiDriver(_audio_driver.get());
#else
	_midi_driver = new DummyMidiDriver();
#endif
	
	// Set event source (FIXME: handle multiple sources)
	_event_source = es;

	_event_source->activate();

	// Create root patch

	Patch* root_patch = new Patch("", 1, NULL,
			_audio_driver->sample_rate(), _audio_driver->buffer_size(), 1);
	root_patch->activate();
	root_patch->add_to_store(_object_store);
	root_patch->process_order(root_patch->build_process_order());
	root_patch->enable();

	assert(_audio_driver->root_patch() == NULL);
	_audio_driver->set_root_patch(root_patch);

	_audio_driver->activate();
#ifdef HAVE_ALSA_MIDI
	_midi_driver->activate();
#endif
	
	_post_processor->start();

	_activated = true;
	
	return true;
}


void
Engine::deactivate()
{
	if (!_activated)
		return;
	
	_audio_driver->root_patch()->disable();
	_audio_driver->root_patch()->deactivate();

	/*for (Tree<GraphObject*>::iterator i = _object_store->objects().begin();
			i != _object_store->objects().end(); ++i)
		if ((*i)->as_node() != NULL && (*i)->as_node()->parent() == NULL)
			(*i)->as_node()->deactivate();*/
	
	if (_midi_driver != NULL)
		_midi_driver->deactivate();
	
	_audio_driver->deactivate();

	// Finalize any lingering events (unlikely)
	_post_processor->whip();
	_post_processor->stop();

	_audio_driver.reset();

	_event_source.reset();
	
	_activated = false;
}


} // namespace Ingen
