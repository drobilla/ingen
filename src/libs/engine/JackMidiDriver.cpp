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
() */

#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <raul/Maid.hpp>
#include <raul/midi_events.h>
#include "types.hpp"
#include "JackMidiDriver.hpp"
#include "ThreadManager.hpp"
#include "AudioDriver.hpp"
#include "MidiBuffer.hpp"
#include "DuplexPort.hpp"
#include "ProcessContext.hpp"
#include "jack_compat.h"
/*#ifdef HAVE_LASH
#include "LashDriver.hpp"
#endif*/
using namespace std;

namespace Ingen {

	
//// JackMidiPort ////

JackMidiPort::JackMidiPort(JackMidiDriver* driver, DuplexPort* patch_port)
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
}


JackMidiPort::~JackMidiPort()
{
	jack_port_unregister(_driver->jack_client(), _jack_port);
}


/** Prepare input for a block before a cycle is run, in the audio thread.
 *
 * This is simple since Jack MIDI is in-band with audio.
 */
void
JackMidiPort::pre_process(ProcessContext& context)
{
	if ( ! is_input() )
		return;
	
	assert(_patch_port->poly() == 1);

	MidiBuffer* patch_buf = dynamic_cast<MidiBuffer*>(_patch_port->buffer(0));
	assert(patch_buf);

	void*                jack_buffer = jack_port_get_buffer(_jack_port, context.nframes());
	const jack_nframes_t event_count = jack_midi_get_event_count(jack_buffer);

	patch_buf->prepare_write(context.nframes());
	
	// Copy events from Jack port buffer into patch port buffer
	for (jack_nframes_t i=0; i < event_count; ++i) {
		jack_midi_event_t ev;
		jack_midi_event_get(&ev, jack_buffer, i);

		bool success = patch_buf->append(ev.time, ev.size, ev.buffer);
		if (!success)
			cerr << "WARNING: Failed to write MIDI to port buffer, event(s) lost!" << endl;
	}

	//if (event_count)
	//	cerr << "Jack MIDI got " << event_count << " events." << endl;
}


/** Prepare output for a block after a cycle is run, in the audio thread.
 *
 * This is simple since Jack MIDI is in-band with audio.
 */
void
JackMidiPort::post_process(ProcessContext& context)
{
	if (is_input())
		return;
	
	assert(_patch_port->poly() == 1);

	MidiBuffer* patch_buf = dynamic_cast<MidiBuffer*>(_patch_port->buffer(0));
	assert(patch_buf);

	void*                jack_buffer = jack_port_get_buffer(_jack_port, context.nframes());
	const jack_nframes_t event_count = patch_buf->event_count();

	patch_buf->prepare_read(context.nframes());

	jack_midi_clear_buffer(jack_buffer);
	
	double         time = 0;
	uint32_t       size = 0;
	unsigned char* data = NULL;

	// Copy events from Jack port buffer into patch port buffer
	for (jack_nframes_t i=0; i < event_count; ++i) {
		patch_buf->get_event(&time, &size, &data);
		jack_midi_event_write(jack_buffer, time, data, size);
	}

	//if (event_count)
	//	cerr << "Jack MIDI wrote " << event_count << " events." << endl;
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
JackMidiDriver::pre_process(ProcessContext& context)
{
	for (Raul::List<JackMidiPort*>::iterator i = _in_ports.begin(); i != _in_ports.end(); ++i)
		(*i)->pre_process(context);
}


/** Write the output from any (top-level, exported) MIDI output ports to Jack ports.
 */
void
JackMidiDriver::post_process(ProcessContext& context)
{
	for (Raul::List<JackMidiPort*>::iterator i = _out_ports.begin(); i != _out_ports.end(); ++i)
		(*i)->post_process(context);
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

