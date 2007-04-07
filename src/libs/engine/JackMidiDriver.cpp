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

#include "JackMidiDriver.h"
#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <raul/Maid.h>
#include <raul/midi_events.h>
#include "types.h"
#include "ThreadManager.h"
#include "AudioDriver.h"
#include "MidiMessage.h"
#include "DuplexPort.h"
#ifdef HAVE_LASH
#include "LashDriver.h"
#endif
using std::cout; using std::cerr; using std::endl;

namespace Ingen {

	
//// JackMidiPort ////

JackMidiPort::JackMidiPort(JackMidiDriver* driver, DuplexPort<MidiMessage>* patch_port)
: DriverPort(patch_port->is_input()),
  Raul::ListNode<JackMidiPort*>(this),
  _driver(driver),
  _jack_port(NULL),
  _patch_port(patch_port)
{
	assert(patch_port->poly() == 1);

	_jack_port = jack_port_register(_driver->jack_client(),
		patch_port->path().c_str(), JACK_DEFAULT_MIDI_TYPE,
		(patch_port->is_input()) ? JackPortIsInput : JackPortIsOutput,
		0);

	patch_port->buffer(0)->clear();
	patch_port->fixed_buffers(true);
}


JackMidiPort::~JackMidiPort()
{
	jack_port_unregister(_driver->jack_client(), _jack_port);
}


/** Prepare events for a block.
 *
 * This is basically trivial (as opposed to AlsaMidiPort) since Jack MIDI
 * data is in-band with the audio thread.
 */
void
JackMidiPort::prepare_block(const SampleCount block_start, const SampleCount block_end)
{
	assert(block_end >= block_start);
	
	const SampleCount    nframes     = block_end - block_start;
	void*                jack_buffer = jack_port_get_buffer(_jack_port, nframes);
	const jack_nframes_t event_count = jack_midi_get_event_count(jack_buffer);
	
	assert(event_count < _patch_port->buffer_size());
	
	// Copy events from Jack port buffer into patch port buffer
	for (jack_nframes_t i=0; i < event_count; ++i) {
		jack_midi_event_t* ev = (jack_midi_event_t*)&_patch_port->buffer(0)->value_at(i);
		jack_midi_event_get(ev, jack_buffer, i);

		// MidiMessage and jack_midi_event_t* are the same thing :/
		MidiMessage* const message = &_patch_port->buffer(0)->data()[i];
		message->time   = ev->time;
		message->size   = ev->size;
		message->buffer = ev->buffer;

		assert(message->time < nframes);
	}

	//cerr << "Jack MIDI got " << event_count << " events." << endl;

	_patch_port->buffer(0)->filled_size(event_count);
	//_patch_port->tied_port()->buffer(0)->filled_size(event_count);
}



//// JackMidiDriver ////


bool JackMidiDriver::_midi_thread_exit_flag = true;


JackMidiDriver::JackMidiDriver(jack_client_t* client)
: _client(client),
  _is_activated(false),
  _is_enabled(false)
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
	_is_activated = true;
}


/** Terminate the MIDI thread.
 */
void
JackMidiDriver::deactivate() 
{
	_is_activated = false;
}


/** Build flat arrays of events to be used as input for the given cycle.
 */
void
JackMidiDriver::prepare_block(const SampleCount block_start, const SampleCount block_end)
{
	for (Raul::List<JackMidiPort*>::iterator i = _in_ports.begin(); i != _in_ports.end(); ++i)
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
JackMidiDriver::add_port(DriverPort* port)
{
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);
	assert(dynamic_cast<JackMidiPort*>(port));

	if (port->is_input())
		_in_ports.push_back((JackMidiPort*)port);
	else
		_out_ports.push_back((JackMidiPort*)port);
}


/** Remove an Jack MIDI port.
 *
 * Realtime safe.  This is to be called at the beginning of a process cycle to
 * remove the port from the lists read by the audio thread, so the port
 * will no longer be used and can be removed afterwards.
 *
 * It is the callers responsibility to delete the returned port.
 */
DriverPort*
JackMidiDriver::remove_port(const Path& path)
{
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);

	// FIXME: duplex?

	for (Raul::List<JackMidiPort*>::iterator i = _in_ports.begin(); i != _in_ports.end(); ++i)
		if ((*i)->patch_port()->path() == path)
			return _in_ports.erase(i)->elem(); // FIXME: LEAK

	for (Raul::List<JackMidiPort*>::iterator i = _out_ports.begin(); i != _out_ports.end(); ++i)
		if ((*i)->patch_port()->path() == path)
			return _out_ports.erase(i)->elem(); // FIXME: LEAK

	cerr << "[JackMidiDriver::remove_input] WARNING: Failed to find Jack port to remove!" << endl;
	return NULL;
}


} // namespace Ingen

