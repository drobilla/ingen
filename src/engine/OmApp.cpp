/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Om.h"
#include "OmApp.h"	
#include "config.h"
#include "tuning.h"
#include <sys/mman.h>
#include <iostream>
#include <unistd.h>
#include "Event.h"
#include "util/Queue.h"
#include "JackAudioDriver.h"
#include "NodeFactory.h"
#include "OSCReceiver.h"
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

namespace Om {


OmApp::OmApp(const char* port)
: m_maid(new Maid(maid_queue_size)),
  m_audio_driver(new JackAudioDriver()),
#ifdef HAVE_JACK_MIDI
  m_midi_driver(new JackMidiDriver(((JackAudioDriver*)m_audio_driver)->jack_client())),
#elif HAVE_ALSA_MIDI
  m_midi_driver(new AlsaMidiDriver()),
#else
  m_midi_driver(new DummyMidiDriver()),
#endif
  m_osc_receiver(new OSCReceiver(pre_processor_queue_size, port)),
  m_client_broadcaster(new ClientBroadcaster()),
  m_object_store(new ObjectStore()),
  m_node_factory(new NodeFactory()),
  m_event_queue(new Queue<Event*>(event_queue_size)),
//  m_pre_processor(new QueuedEventSource(pre_processor_queue_size)),
  m_post_processor(new PostProcessor(post_processor_queue_size)),
  m_quit_flag(false),
  m_activated(false)
{
	mlock(m_audio_driver, sizeof(JackAudioDriver));
	mlock(m_object_store, sizeof(ObjectStore));
	mlock(m_osc_receiver, sizeof(OSCReceiver));
#ifdef HAVE_ALSA_MIDI
	mlock(m_midi_driver, sizeof(AlsaMidiDriver));
#else
	mlock(m_midi_driver, sizeof(DummyMidiDriver));
#endif
	
	m_osc_receiver->start();
	m_post_processor->start();
}


OmApp::OmApp(const char* port, AudioDriver* audio_driver)
: m_maid(new Maid(maid_queue_size)),
  m_audio_driver(audio_driver),
#ifdef HAVE_JACK_MIDI
  m_midi_driver(new JackMidiDriver(((JackAudioDriver*)m_audio_driver)->jack_client())),
#elif HAVE_ALSA_MIDI
  m_midi_driver(new AlsaMidiDriver()),
#else
  m_midi_driver(new DummyMidiDriver()),
#endif
  m_osc_receiver(new OSCReceiver(pre_processor_queue_size, port)),
  m_client_broadcaster(new ClientBroadcaster()),
  m_object_store(new ObjectStore()),
  m_node_factory(new NodeFactory()),
  m_event_queue(new Queue<Event*>(event_queue_size)),
  //m_pre_processor(new QueuedEventSource(pre_processor_queue_size)),
  m_post_processor(new PostProcessor(post_processor_queue_size)),
  m_quit_flag(false),
  m_activated(false)
{
	mlock(m_audio_driver, sizeof(JackAudioDriver));
	mlock(m_object_store, sizeof(ObjectStore));
	mlock(m_osc_receiver, sizeof(OSCReceiver));
#ifdef HAVE_ALSA_MIDI
	mlock(m_midi_driver, sizeof(AlsaMidiDriver));
#else
	mlock(m_midi_driver, sizeof(DummyMidiDriver));
#endif
	
	m_osc_receiver->start();
	m_post_processor->start();
}


OmApp::~OmApp()
{
	deactivate();

	for (Tree<OmObject*>::iterator i = m_object_store->objects().begin();
			i != m_object_store->objects().end(); ++i) {
		if ((*i)->parent() == NULL)
			delete (*i);
	}
	
	delete m_object_store;
	delete m_client_broadcaster;
	delete m_osc_receiver;
	delete m_node_factory;
	delete m_midi_driver;
	delete m_audio_driver;
	
	delete m_maid;

	munlockall();
}


/* driver() template specializations.
 * Due to the lack of RTTI, this needs to be implemented manually like this.
 * If more types/drivers start getting added, it may be worth it to enable
 * RTTI and put all the drivers into a map with typeid's as the key.  That's
 * more elegant and extensible, but this is faster and simpler - for now.
 */
template<>
Driver<MidiMessage>* OmApp::driver<MidiMessage>() { return m_midi_driver; }
template<>
Driver<sample>* OmApp::driver<sample>() { return m_audio_driver; }


int
OmApp::main()
{
	// Loop until quit flag is set (by OSCReceiver)
	while ( ! m_quit_flag) {
		nanosleep(&main_rate, NULL);
#ifdef HAVE_LASH
		// Process any pending LASH events
		if (lash_driver->enabled())
			lash_driver->process_events();
#endif
		// Run the maid (garbage collector)
		m_maid->cleanup();
	}
	cout << "[Main] Done main loop." << endl;
	
	if (m_activated)
		deactivate();

	sleep(1);
	cout << "[Main] Om exiting..." << endl;
	
	return 0;
}


void
OmApp::activate()
{
	if (m_activated)
		return;
	
	// Create root patch
	CreatePatchEvent create_ev(CountedPtr<Responder>(new Responder()), "/", 1);
	create_ev.pre_process();
	create_ev.execute(0);
	create_ev.post_process();
	EnablePatchEvent enable_ev(CountedPtr<Responder>(new Responder()), "/");
	enable_ev.pre_process();
	enable_ev.execute(0);
	enable_ev.post_process();

	assert(m_audio_driver->root_patch() != NULL);

	m_audio_driver->activate();
#ifdef HAVE_ALSA_MIDI
	m_midi_driver->activate();
#endif
	m_activated = true;
}


void
OmApp::deactivate()
{
	if (!m_activated)
		return;
	
	m_audio_driver->root_patch()->process(false);

	for (Tree<OmObject*>::iterator i = m_object_store->objects().begin();
			i != m_object_store->objects().end(); ++i)
		if ((*i)->as_node() != NULL && (*i)->as_node()->parent() == NULL)
			(*i)->as_node()->deactivate();
	
	if (m_midi_driver != NULL)
		m_midi_driver->deactivate();
	
	m_osc_receiver->stop();
	m_audio_driver->deactivate();

	m_activated = false;
}


} // namespace Om
