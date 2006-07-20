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

#include "JackAudioDriver.h"
#include "config.h"
#include "tuning.h"
#include <iostream>
#include <cstdlib>
#include "Engine.h"
#include "util.h"
#include "Event.h"
#include "QueuedEvent.h"
#include "EventSource.h"
#include "OSCReceiver.h"
#include "PostProcessor.h"
#include "util/Queue.h"
#include "Node.h"
#include "Patch.h"
#include "Port.h"
#include "MidiDriver.h"
#include "List.h"
#include "DuplexPort.h"
#ifdef HAVE_LASH
#include "LashDriver.h"
#endif

using std::cout; using std::cerr; using std::endl;


namespace Ingen {

	
//// JackAudioPort ////

JackAudioPort::JackAudioPort(JackAudioDriver* driver, DuplexPort<Sample>* patch_port)
: DriverPort(),
  ListNode<JackAudioPort*>(this),
  m_driver(driver),
  m_jack_port(NULL),
  m_jack_buffer(NULL),
  m_patch_port(patch_port)
{
	//assert(patch_port->tied_port() != NULL);
	assert(patch_port->poly() == 1);

	m_jack_port = jack_port_register(m_driver->jack_client(),
		patch_port->path().c_str(), JACK_DEFAULT_AUDIO_TYPE,
		(patch_port->is_input()) ? JackPortIsInput : JackPortIsOutput,
		0);

	patch_port->fixed_buffers(true);
}	


JackAudioPort::~JackAudioPort()
{
	jack_port_unregister(m_driver->jack_client(), m_jack_port);
}


void
JackAudioPort::add_to_driver()
{
	m_driver->add_port(this);
}


void
JackAudioPort::remove_from_driver()
{
	m_driver->remove_port(this);
}

void
JackAudioPort::prepare_buffer(jack_nframes_t nframes)
{
	// FIXME: Technically this doesn't need to be done every time for output ports
	/*m_jack_buffer->set_data((jack_default_audio_sample_t*)
		jack_port_get_buffer(m_jack_port, nframes));
	
	m_patch_port->buffer(0)->join(m_jack_buffer);
*/
	jack_sample_t* jack_buf = (jack_sample_t*)jack_port_get_buffer(m_jack_port, nframes);

	if (jack_buf != m_jack_buffer) {
		//cerr << "Jack buffer: " << jack_buf << endl;
		m_patch_port->buffer(0)->set_data(jack_buf);
		m_jack_buffer = jack_buf;
	}

	//assert(m_patch_port->tied_port() != NULL);
	
	// FIXME: fixed_buffers switch on/off thing can be removed once this
	// gets figured out and assertions can go away
	//m_patch_port->fixed_buffers(false);
	//m_patch_port->buffer(0)->join(m_jack_buffer);
	//m_patch_port->tied_port()->buffer(0)->join(m_jack_buffer);

	//m_patch_port->fixed_buffers(true);
	
	//assert(m_patch_port->buffer(0)->data() == m_patch_port->tied_port()->buffer(0)->data());
	assert(m_patch_port->buffer(0)->data() == jack_buf);
}

	
//// JackAudioDriver ////

JackAudioDriver::JackAudioDriver()
: m_client(NULL),
  m_buffer_size(0),
  m_sample_rate(0),
  m_is_activated(false),
  m_local_client(true),
  m_root_patch(NULL)
{
	m_client = jack_client_new("Ingen");
	if (m_client == NULL) {
		cerr << "[JackAudioDriver] Unable to connect to Jack.  Exiting." << endl;
		exit(EXIT_FAILURE);
	}

	jack_on_shutdown(m_client, shutdown_cb, this);

	m_buffer_size = jack_get_buffer_size(m_client);
	m_sample_rate = jack_get_sample_rate(m_client);

	jack_set_sample_rate_callback(m_client, sample_rate_cb, this);
	jack_set_buffer_size_callback(m_client, buffer_size_cb, this);
}

JackAudioDriver::JackAudioDriver(jack_client_t* jack_client)
: m_client(jack_client),
  m_buffer_size(jack_get_buffer_size(jack_client)),
  m_sample_rate(jack_get_sample_rate(jack_client)),
  m_is_activated(false),
  m_local_client(false)
{
	jack_on_shutdown(m_client, shutdown_cb, this);

	jack_set_sample_rate_callback(m_client, sample_rate_cb, this);
	jack_set_buffer_size_callback(m_client, buffer_size_cb, this);
}


JackAudioDriver::~JackAudioDriver()
{
	deactivate();
	
	if (m_local_client)
		jack_client_close(m_client);
}


void
JackAudioDriver::activate()
{	
	if (m_is_activated) {
		cerr << "[JackAudioDriver] Jack driver already activated." << endl;
		return;
	}

	jack_set_process_callback(m_client, process_cb, this);

	m_is_activated = true;

	if (jack_activate(m_client)) {
		cerr << "[JackAudioDriver] Could not activate Jack client, aborting." << endl;
		exit(EXIT_FAILURE);
	} else {
		cout << "[JackAudioDriver] Activated Jack client." << endl;
#ifdef HAVE_LASH
	Engine::instance().lash_driver()->set_jack_client_name(jack_client_get_name(m_client));
#endif
	}
}


void
JackAudioDriver::deactivate()
{
	if (m_is_activated) {
		Engine::instance().osc_receiver()->deactivate();
	
		jack_deactivate(m_client);
		m_is_activated = false;
	
		for (List<JackAudioPort*>::iterator i = m_ports.begin(); i != m_ports.end(); ++i)
			jack_port_unregister(m_client, (*i)->jack_port());
	
		m_ports.clear();
	
		cout << "[JackAudioDriver] Deactivated Jack client." << endl;
		
		Engine::instance().post_processor()->stop();
	}
}


/** Add a Jack port.
 *
 * Realtime safe, this is to be called at the beginning of a process cycle to
 * insert (and actually begin using) a new port.
 * 
 * See create_port() and remove_port().
 */
void
JackAudioDriver::add_port(JackAudioPort* port)
{
	m_ports.push_back(port);
}


/** Remove a Jack port.
 *
 * Realtime safe.  This is to be called at the beginning of a process cycle to
 * remove the port from the lists read by the audio thread, so the port
 * will no longer be used and can be removed afterwards.
 *
 * It is the callers responsibility to delete the returned port.
 */
JackAudioPort*
JackAudioDriver::remove_port(JackAudioPort* port)
{
	for (List<JackAudioPort*>::iterator i = m_ports.begin(); i != m_ports.end(); ++i)
		if ((*i) == port)
			return m_ports.remove(i)->elem();
	
	cerr << "[JackAudioDriver::remove_port] WARNING: Failed to find Jack port to remove!" << endl;
	return NULL;
}


DriverPort*
JackAudioDriver::create_port(DuplexPort<Sample>* patch_port)
{
	if (patch_port->buffer_size() == m_buffer_size)
		return new JackAudioPort(this, patch_port);
	else
		return NULL;
}


/** Process all the pending events for this cycle.
 *
 * Called from the realtime thread once every process cycle.
 */
void
JackAudioDriver::process_events(jack_nframes_t block_start, jack_nframes_t block_end)
{
	Event* ev = NULL;

	/* Limit the maximum number of queued events to process per cycle.  This
	 * makes the process callback (more) realtime-safe by preventing being
	 * choked by events coming in faster than they can be processed.
	 * FIXME: run the math on this and figure out a good value */
	const unsigned int MAX_QUEUED_EVENTS = m_buffer_size / 100;

	unsigned int num_events_processed = 0;
	unsigned int offset = 0;
	
	// Process the "slow" events first, because it's possible some of the
	// RT events depend on them
	
	/* FIXME: Merge these next two loops into one */

	// FIXME
	while ((ev = Engine::instance().osc_receiver()->pop_earliest_queued_before(block_end))) {
		ev->execute(0);  // QueuedEvents are not sample accurate
		Engine::instance().post_processor()->push(ev);
		if (++num_events_processed > MAX_QUEUED_EVENTS)
			break;
	}
	
	while ((ev = Engine::instance().osc_receiver()->pop_earliest_stamped_before(block_end))) {
		if (ev->time_stamp() >= block_start)
			offset = ev->time_stamp() - block_start;
		else
			offset = 0;

		ev->execute(offset);
		Engine::instance().post_processor()->push(ev);
		++num_events_processed;
	}
	
	if (num_events_processed > 0)
		Engine::instance().post_processor()->whip();
}



/**** Jack Callbacks ****/



/** Jack process callback, drives entire audio thread.
 *
 * \callgraph
 */
int
JackAudioDriver::m_process_cb(jack_nframes_t nframes) 
{
	static jack_nframes_t start_of_current_cycle = 0;
	static jack_nframes_t start_of_last_cycle    = 0;

	// FIXME: support nframes != buffer_size, even though that never damn well happens
	assert(nframes == m_buffer_size);

	// Jack can elect to not call this function for a cycle, if overloaded
	// FIXME: this doesn't make sense, and the start time isn't used anyway
	start_of_current_cycle = jack_last_frame_time(m_client);
	start_of_last_cycle = start_of_current_cycle - nframes;

	// FIXME: ditto
	assert(start_of_current_cycle - start_of_last_cycle == nframes);

	m_transport_state = jack_transport_query(m_client, &m_position);
	
	process_events(start_of_last_cycle, start_of_current_cycle);
	Engine::instance().midi_driver()->prepare_block(start_of_last_cycle, start_of_current_cycle);
	
	// Set buffers of patch ports to Jack port buffers (zero-copy processing)
	for (List<JackAudioPort*>::iterator i = m_ports.begin(); i != m_ports.end(); ++i)
		(*i)->prepare_buffer(nframes);
	
	// Run root patch
	assert(m_root_patch != NULL);
	m_root_patch->process(nframes);
	
	return 0;
}


void 
JackAudioDriver::m_shutdown_cb() 
{
	cout << "[JackAudioDriver] Jack shutdown.  Exiting." << endl;
	Engine::instance().quit();
}


int
JackAudioDriver::m_sample_rate_cb(jack_nframes_t nframes) 
{
	if (m_is_activated) {
		cerr << "[JackAudioDriver] On-the-fly sample rate changing not supported (yet).  Aborting." << endl;
		exit(EXIT_FAILURE);
	} else {
		m_sample_rate = nframes;
	}
	return 0;
}


int
JackAudioDriver::m_buffer_size_cb(jack_nframes_t nframes) 
{
	if (m_is_activated) {
		cerr << "[JackAudioDriver] On-the-fly buffer size changing not supported (yet).  Aborting." << endl;
		exit(EXIT_FAILURE);
	} else {
		m_buffer_size = nframes;
	}
	return 0;
}


} // namespace Ingen

