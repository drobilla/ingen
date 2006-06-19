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

#include "JackMidiDriver.h"
#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include "Om.h"
#include "OmApp.h"
#include "types.h"
#include "midi.h"
#include "OmApp.h"
#include "Maid.h"
#include "AudioDriver.h"
#include "MidiMessage.h"
#include "TypedPort.h"
#ifdef HAVE_LASH
#include "LashDriver.h"
#endif
using std::cout; using std::cerr; using std::endl;

namespace Om {

	
//// JackMidiPort ////

JackMidiPort::JackMidiPort(JackMidiDriver* driver, TypedPort<MidiMessage>* patch_port)
: DriverPort(),
  ListNode<JackMidiPort*>(this),
  m_driver(driver),
  m_jack_port(NULL),
  m_patch_port(patch_port)
{
	assert(patch_port->poly() == 1);

	m_jack_port = jack_port_register(m_driver->jack_client(),
		patch_port->path().c_str(), JACK_DEFAULT_MIDI_TYPE,
		(patch_port->is_input()) ? JackPortIsInput : JackPortIsOutput,
		0);

	patch_port->buffer(0)->clear();
	patch_port->fixed_buffers(true);
}


JackMidiPort::~JackMidiPort()
{
	jack_port_unregister(m_driver->jack_client(), m_jack_port);
}


void
JackMidiPort::add_to_driver()
{
	m_driver->add_port(this);
}


void
JackMidiPort::remove_from_driver()
{
	m_driver->remove_port(this);
}


/** Prepare events for a block.
 *
 * This is basically trivial (as opposed to AlsaMidiPort) since Jack MIDI
 * data is in-band with the audio thread.
 *
 * Prepares all events that occurred during the time interval passed
 * (which ideally are the events from the previous cycle with an exact
 * 1 cycle delay).
 */
void
JackMidiPort::prepare_block(const samplecount block_start, const samplecount block_end)
{
	assert(block_end >= block_start);
	
	const samplecount    nframes     = block_end - block_start;
	void*                jack_buffer = jack_port_get_buffer(m_jack_port, nframes);
	const jack_nframes_t event_count = jack_midi_port_get_info(jack_buffer, nframes)->event_count;
	
	assert(event_count < m_patch_port->buffer_size());
	
	// Copy events from Jack port buffer into patch port buffer
	for (jack_nframes_t i=0; i < event_count; ++i) {
		jack_midi_event_t* ev = (jack_midi_event_t*)&m_patch_port->buffer(0)->value_at(i);
		jack_midi_event_get(ev, jack_buffer, i, nframes);

		// Convert note ons with velocity 0 to proper note offs
		if (ev->buffer[0] == MIDI_CMD_NOTE_ON && ev->buffer[2] == 0)
			ev->buffer[0] = MIDI_CMD_NOTE_OFF;
		
		// MidiMessage and jack_midi_event_t* are the same thing :/
		MidiMessage* const message = &m_patch_port->buffer(0)->data()[i];
		message->time   = ev->time;
		message->size   = ev->size;
		message->buffer = ev->buffer;
	}

	//cerr << "Jack MIDI got " << event_count << " events." << endl;

	m_patch_port->buffer(0)->filled_size(event_count);
	m_patch_port->tied_port()->buffer(0)->filled_size(event_count);
}



//// JackMidiDriver ////


bool JackMidiDriver::m_midi_thread_exit_flag = true;


JackMidiDriver::JackMidiDriver(jack_client_t* client)
: m_client(client),
  m_is_activated(false),
  m_is_enabled(false)
{
}


JackMidiDriver::~JackMidiDriver()
{
	deactivate();
}


/** Launch and start the MIDI thread.
 */
void
JackMidiDriver::activate() 
{
	m_is_activated = true;
}


/** Terminate the MIDI thread.
 */
void
JackMidiDriver::deactivate() 
{
	m_is_activated = false;
}


/** Build flat arrays of events for DSSI plugins for each Port.
 */
void
JackMidiDriver::prepare_block(const samplecount block_start, const samplecount block_end)
{
	for (List<JackMidiPort*>::iterator i = m_in_ports.begin(); i != m_in_ports.end(); ++i)
		(*i)->prepare_block(block_start, block_end);
}


/** Add an Jack MIDI port.
 *
 * Realtime safe, this is to be called at the beginning of a process cycle to
 * insert (and actually begin using) a new port.
 * 
 * See create_port() and remove_port().
 */
void
JackMidiDriver::add_port(JackMidiPort* port)
{
	if (port->patch_port()->is_input())
		m_in_ports.push_back(port);
	else
		m_out_ports.push_back(port);
}


/** Remove an Jack MIDI port.
 *
 * Realtime safe.  This is to be called at the beginning of a process cycle to
 * remove the port from the lists read by the audio thread, so the port
 * will no longer be used and can be removed afterwards.
 *
 * It is the callers responsibility to delete the returned port.
 */
JackMidiPort*
JackMidiDriver::remove_port(JackMidiPort* port)
{
	if (port->patch_port()->is_input()) {
		for (List<JackMidiPort*>::iterator i = m_in_ports.begin(); i != m_in_ports.end(); ++i)
			if ((*i) == (JackMidiPort*)port)
				return m_in_ports.remove(i)->elem();
	} else {
		for (List<JackMidiPort*>::iterator i = m_out_ports.begin(); i != m_out_ports.end(); ++i)
			if ((*i) == port)
				return m_out_ports.remove(i)->elem();
	}
	
	cerr << "[JackMidiDriver::remove_input] WARNING: Failed to find Jack port to remove!" << endl;
	return NULL;
}


} // namespace Om

