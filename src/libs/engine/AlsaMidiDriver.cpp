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

#include "AlsaMidiDriver.h"
#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include "Om.h"
#include "OmApp.h"
#include "types.h"
#include "OmApp.h"
#include "Maid.h"
#include "AudioDriver.h"
#include "MidiMessage.h"
#include "DuplexPort.h"
#ifdef HAVE_LASH
#include "LashDriver.h"
#endif
using std::cout; using std::cerr; using std::endl;

namespace Om {

	
//// AlsaMidiPort ////

AlsaMidiPort::AlsaMidiPort(AlsaMidiDriver* driver, DuplexPort<MidiMessage>* port)
: DriverPort(),
  ListNode<AlsaMidiPort*>(this),
  m_driver(driver),
  m_patch_port(port),
  m_port_id(0),
  m_midi_pool(new unsigned char*[port->buffer_size()]),
  m_events(1024)
{
	assert(port->parent() != NULL);
	assert(port->poly() == 1);

	if (port->is_input()) {
		if ((m_port_id = snd_seq_create_simple_port(driver->seq_handle(), port->path().c_str(),
   	 		SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
			SND_SEQ_PORT_TYPE_APPLICATION)) < 0)
		{
			cerr << "[AlsaMidiPort] Error creating sequencer port." << endl;
			exit(EXIT_FAILURE);
		}
	} else {
		if ((m_port_id = snd_seq_create_simple_port(driver->seq_handle(), port->path().c_str(),
   	 		SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
			SND_SEQ_PORT_TYPE_APPLICATION)) < 0)
		{
			cerr << "[AlsaMidiPort] Error creating sequencer port." << endl;
			exit(EXIT_FAILURE);
		}
	}

	/* Allocate event pool.  This pool is used when preparing a block from the queue
	 * of Alsa events.  The buffer member of the MidiMessage's in the patch port's
	 * buffer will be set directly to an element in this pool, then next cycle they
	 * will be overwritten (eliminating the need for any allocation/freeing). */
	for (size_t i=0; i < port->buffer_size(); ++i)
		m_midi_pool[i] = new unsigned char[MAX_MIDI_EVENT_SIZE];

	port->buffer(0)->clear();
	port->fixed_buffers(true);
}


AlsaMidiPort::~AlsaMidiPort()
{
	snd_seq_delete_simple_port(m_driver->seq_handle(), m_port_id);

	// Free event pool
	for (size_t i=0; i < m_patch_port->buffer_size(); ++i)
		delete[] m_midi_pool[i];
	
	delete[] m_midi_pool;
}


void
AlsaMidiPort::add_to_driver()
{
	m_driver->add_port(this);
}


void
AlsaMidiPort::remove_from_driver()
{
	m_driver->remove_port(this);
}


void
AlsaMidiPort::set_name(const string& name)
{
	snd_seq_port_info_t* info = NULL;
	snd_seq_port_info_malloc(&info);
	snd_seq_get_port_info(m_driver->seq_handle(), m_port_id, info);
	snd_seq_port_info_set_name(info, name.c_str());
	snd_seq_set_port_info(m_driver->seq_handle(), m_port_id, info);
	snd_seq_port_info_free(info);
}


void
AlsaMidiPort::event(snd_seq_event_t* const ev)
{
	// Abuse the tick field to hold the timestamp
	ev->time.tick = om->audio_driver()->time_stamp();
	
	// Fix noteons with velocity 0 (required for DSSI spec)
	if (ev->type == SND_SEQ_EVENT_NOTEON && ev->data.note.velocity == 0)
		ev->type = SND_SEQ_EVENT_NOTEOFF;

	m_events.push(*ev);
}


/** Generates a flat array of MIDI events for patching.
 *
 * Prepares all events that occurred during the time interval passed
 * (which ideally are the events from the previous cycle with an exact
 * 1 cycle delay) and creates a flat port buffer for this cycle.
 */
void
AlsaMidiPort::prepare_block(const samplecount block_start, const samplecount block_end)
{
	assert(block_end >= block_start);
	
	snd_seq_event_t* ev         = NULL;
	MidiMessage*     message    = NULL;
	size_t           num_events = 0;
	size_t           event_size = 0; // decoded length of Alsa event in bytes
	int              timestamp  = 0;
	
	while (!m_events.is_empty() && m_events.front().time.tick < block_end) {
		assert(num_events < m_patch_port->buffer_size());
		ev = &m_events.front();
		message = &m_patch_port->buffer(0)->data()[num_events];
		
		timestamp = ev->time.tick - block_start;
		if (timestamp < 0) {
			// FIXME: remove this (obviously not realtime safe)
			cerr << "[AlsaMidiPort] Missed event by " << -timestamp << " samples!" << endl;
			timestamp = 0;
		}
		assert(timestamp < (int)(block_end - block_start));
		
		// Reset decoder so we don't get running status
		snd_midi_event_reset_decode(m_driver->event_coder());

		// FIXME: is this realtime safe?
		if ((event_size = snd_midi_event_decode(m_driver->event_coder(),
				m_midi_pool[num_events], MAX_MIDI_EVENT_SIZE, ev)) > 0) {
			message->size = event_size;
			message->time = timestamp;
			message->buffer = m_midi_pool[num_events];
			++num_events;
		} else {
			cerr << "[AlsaMidiPort] Unable to decode MIDI event" << endl;
		}
		
		m_events.pop();
	}

	m_patch_port->buffer(0)->filled_size(num_events);
	//m_patch_port->tied_port()->buffer(0)->filled_size(num_events);
}



//// AlsaMidiDriver ////


bool AlsaMidiDriver::m_midi_thread_exit_flag = true;


AlsaMidiDriver::AlsaMidiDriver()
: m_seq_handle(NULL),
  m_event_coder(NULL),
  m_is_activated(false)
{
	if (snd_seq_open(&m_seq_handle, "hw", SND_SEQ_OPEN_INPUT, 0) < 0) {
		cerr << "[AlsaMidiDriver] Error opening ALSA sequencer." << endl;
		exit(EXIT_FAILURE);
	} else {
		cout << "[AlsaMidiDriver] Successfully opened ALSA sequencer." << endl;
	}

	if (snd_midi_event_new(3, &m_event_coder)) {
		cerr << "[AlsaMidiDriver] Failed to initialize ALSA MIDI event coder!";
		exit(EXIT_FAILURE);
	} else {
		snd_midi_event_reset_encode(m_event_coder);
		snd_midi_event_reset_decode(m_event_coder);
	}

	snd_seq_set_client_name(m_seq_handle, "Om");
}


AlsaMidiDriver::~AlsaMidiDriver()
{
	deactivate();
	snd_midi_event_free(m_event_coder);
	snd_seq_close(m_seq_handle);
}


/** Launch and start the MIDI thread.
 */
void
AlsaMidiDriver::activate() 
{
	// Just exit if already running
	if (m_midi_thread_exit_flag == false)
		return;
	
	bool success = false;
	m_midi_thread_exit_flag = false;

	//if (om->audio_driver()->is_realtime()) {
		pthread_attr_t attr;
		pthread_attr_init(&attr);

		if (pthread_attr_setschedpolicy(&attr, SCHED_FIFO)) {
			cerr << "[AlsaMidiDriver] Unable to set realtime scheduling for MIDI thread." << endl;
		}

		sched_param param;
		param.sched_priority = 10;

		pthread_attr_setstacksize(&attr, 1500000);

		if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED)
				|| pthread_attr_setschedparam(&attr, &param))
			cout << "[AlsaMidiDriver] Unable to set SCHED_FIFO priority "
				<< param.sched_priority << endl;
	 
		if (!pthread_create(&m_process_thread, &attr, process_midi_in, this)) {
			cout << "[AlsaMidiDriver] Started realtime MIDI thread (SCHED_FIFO, priority "
				<< param.sched_priority << ")" << endl;
			success = true;
		} else { 
			cerr << "[AlsaMidiDriver] Unable to start realtime MIDI thread." << endl;
		}
		pthread_attr_destroy(&attr);
	//}
	
	if (!success) {
		// FIXME: check for success
		pthread_create(&m_process_thread, NULL, process_midi_in, this);
		cout << "[AlsaMidiDriver] Started non-realtime MIDI thread." << endl;
	}
	
#ifdef HAVE_LASH
	om->lash_driver()->set_alsa_client_id(snd_seq_client_id(m_seq_handle));
#endif

	m_is_activated = true;
}


/** Terminate the MIDI thread.
 */
void
AlsaMidiDriver::deactivate() 
{
	if (m_is_activated) {
		m_midi_thread_exit_flag = true;
		pthread_cancel(m_process_thread);
		pthread_join(m_process_thread, NULL);
		m_is_activated = false;
	}
}


/** Build flat arrays of events for DSSI plugins for each Port.
 */
void
AlsaMidiDriver::prepare_block(const samplecount block_start, const samplecount block_end)
{
	for (List<AlsaMidiPort*>::iterator i = m_in_ports.begin(); i != m_in_ports.end(); ++i)
		(*i)->prepare_block(block_start, block_end);
}


/** Add an Alsa MIDI port.
 *
 * Realtime safe, this is to be called at the beginning of a process cycle to
 * insert (and actually begin using) a new port.
 * 
 * See create_port() and remove_port().
 */
void
AlsaMidiDriver::add_port(AlsaMidiPort* port)
{
	if (port->patch_port()->is_input())
		m_in_ports.push_back(port);
	else
		m_out_ports.push_back(port);
}


/** Remove an Alsa MIDI port.
 *
 * Realtime safe.  This is to be called at the beginning of a process cycle to
 * remove the port from the lists read by the audio thread, so the port
 * will no longer be used and can be removed afterwards.
 *
 * It is the callers responsibility to delete the returned port.
 */
AlsaMidiPort*
AlsaMidiDriver::remove_port(AlsaMidiPort* port)
{
	if (port->patch_port()->is_input()) {
		for (List<AlsaMidiPort*>::iterator i = m_in_ports.begin(); i != m_in_ports.end(); ++i)
			if ((*i) == (AlsaMidiPort*)port)
				return m_in_ports.remove(i)->elem();
	} else {
		for (List<AlsaMidiPort*>::iterator i = m_out_ports.begin(); i != m_out_ports.end(); ++i)
			if ((*i) == port)
				return m_out_ports.remove(i)->elem();
	}
	
	cerr << "[AlsaMidiDriver::remove_input] WARNING: Failed to find Jack port to remove!" << endl;
	return NULL;
}


/** MIDI thread.
 */
void*
AlsaMidiDriver::process_midi_in(void* alsa_driver) 
{
	AlsaMidiDriver* ad = (AlsaMidiDriver*)alsa_driver;

	snd_seq_event_t* ev;

	int npfd = snd_seq_poll_descriptors_count(ad->m_seq_handle, POLLIN);
	struct pollfd pfd;
	snd_seq_poll_descriptors(ad->m_seq_handle, &pfd, npfd, POLLIN);
	
	while ( ! m_midi_thread_exit_flag)
		if (poll(&pfd, npfd, 100000) > 0)
			while (snd_seq_event_input(ad->m_seq_handle, &ev) > 0)
				for (List<AlsaMidiPort*>::iterator i = ad->m_in_ports.begin(); i != ad->m_in_ports.end(); ++i)
					if ((*i)->port_id() == ev->dest.port)
						(*i)->event(ev);
	
	cout << "[AlsaMidiDriver] Exiting MIDI thread." << endl;

	return NULL;
}


} // namespace Om

