/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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
#include "raul/Maid.hpp"
#include "raul/midi_events.h"
#include "module/World.hpp"
#include "shared/LV2Features.hpp"
#include "event.lv2/event-helpers.h"
#include "shared/LV2URIMap.hpp"
#include "JackMidiDriver.hpp"
#include "JackAudioDriver.hpp"
#include "ThreadManager.hpp"
#include "AudioDriver.hpp"
#include "EventBuffer.hpp"
#include "DuplexPort.hpp"
#include "ProcessContext.hpp"
#include "Engine.hpp"
#include "util.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {


//// JackMidiPort ////

JackMidiPort::JackMidiPort(JackMidiDriver* driver, DuplexPort* patch_port)
	: DriverPort(patch_port)
	, Raul::List<JackMidiPort*>::Node(this)
	, _driver(driver)
	, _jack_port(NULL)
{
	assert(patch_port->poly() == 1);

	create();

	patch_port->buffer(0)->clear();
}


JackMidiPort::~JackMidiPort()
{
	assert(_jack_port == NULL);
}


void
JackMidiPort::create()
{
	_jack_port = jack_port_register(_driver->jack_client(),
		ingen_jack_port_name(_patch_port->path()).c_str(),
		JACK_DEFAULT_MIDI_TYPE,
		(_patch_port->is_input()) ? JackPortIsInput : JackPortIsOutput,
		0);

	if (_jack_port == NULL) {
		cerr << "[JackMidiPort] ERROR: Failed to register port " << _patch_port->path() << endl;
		throw JackAudioDriver::PortRegistrationFailedException();
	}
}


void
JackMidiPort::destroy()
{
	assert(_jack_port);
	if (jack_port_unregister(_driver->jack_client(), _jack_port))
		cerr << "[JackMidiPort] ERROR: Unable to unregister port" << endl;

	_jack_port = NULL;
}


void
JackMidiPort::move(const Raul::Path& path)
{
	jack_port_set_name(_jack_port, ingen_jack_port_name(path).c_str());
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

	EventBuffer* patch_buf = dynamic_cast<EventBuffer*>(_patch_port->buffer(0).get());
	assert(patch_buf);

	void*                jack_buffer = jack_port_get_buffer(_jack_port, context.nframes());
	const jack_nframes_t event_count = jack_midi_get_event_count(jack_buffer);

	patch_buf->prepare_write(context);

	// Copy events from Jack port buffer into patch port buffer
	for (jack_nframes_t i=0; i < event_count; ++i) {
		jack_midi_event_t ev;
		jack_midi_event_get(&ev, jack_buffer, i);

		const bool success = patch_buf->append(ev.time, 0, _driver->_midi_event_type,
				ev.size, ev.buffer);
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

	EventBuffer* patch_buf = dynamic_cast<EventBuffer*>(_patch_port->buffer(0).get());
	void*        jack_buf  = jack_port_get_buffer(_jack_port, context.nframes());

	assert(_patch_port->poly() == 1);
	assert(patch_buf);

	patch_buf->prepare_read(context);
	jack_midi_clear_buffer(jack_buf);

	uint32_t frames = 0;
	uint32_t subframes = 0;
	uint16_t type = 0;
	uint16_t size = 0;
	uint8_t* data = NULL;

	// Copy events from Jack port buffer into patch port buffer
	for (patch_buf->rewind(); patch_buf->is_valid(); patch_buf->increment()) {
		patch_buf->get_event(&frames, &subframes, &type, &size, &data);
		jack_midi_event_write(jack_buf, frames, data, size);
	}

	//if (event_count)
	//	cerr << "Jack MIDI wrote " << event_count << " events." << endl;
}



//// JackMidiDriver ////


bool JackMidiDriver::_midi_thread_exit_flag = true;


JackMidiDriver::JackMidiDriver(Engine& engine)
	: _engine(engine)
	, _client(NULL)
	, _is_activated(false)
	, _is_enabled(false)
{
	SharedPtr<Shared::LV2URIMap> map = PtrCast<Shared::LV2URIMap>(
			_engine.world()->lv2_features->feature(LV2_URI_MAP_URI));
	_midi_event_type = map->uri_to_id(NULL, "http://lv2plug.in/ns/ext/midi#MidiEvent");
}


JackMidiDriver::~JackMidiDriver()
{
	deactivate();
}


void
JackMidiDriver::attach(AudioDriver& driver)
{
	JackAudioDriver* jad = dynamic_cast<JackAudioDriver*>(&driver);
	assert(jad);
	_client = jad->jack_client();
	for (Raul::List<JackMidiPort*>::iterator i = _ports.begin(); i != _ports.end(); ++i)
		(*i)->create();
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
	for (Raul::List<JackMidiPort*>::iterator i = _ports.begin(); i != _ports.end(); ++i)
		(*i)->destroy();
	_is_activated = false;
}


/** Build flat arrays of events to be used as input for the given cycle.
 */
void
JackMidiDriver::pre_process(ProcessContext& context)
{
	for (Raul::List<JackMidiPort*>::iterator i = _ports.begin(); i != _ports.end(); ++i)
		if ((*i)->is_input())
			(*i)->pre_process(context);
}


/** Write the output from any (top-level, exported) MIDI output ports to Jack ports.
 */
void
JackMidiDriver::post_process(ProcessContext& context)
{
	for (Raul::List<JackMidiPort*>::iterator i = _ports.begin(); i != _ports.end(); ++i)
		if (!(*i)->is_input())
			(*i)->post_process(context);
}


/** Add an Jack MIDI port.
 *
 * Realtime safe, this is to be called at the beginning of a process cycle to
 * insert (and actually begin using) a new port.
 *
 * See new_port() and remove_port().
 */
void
JackMidiDriver::add_port(DriverPort* port)
{
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);
	assert(dynamic_cast<JackMidiPort*>(port));
	_ports.push_back((JackMidiPort*)port);
}


/** Remove a Jack MIDI port.
 *
 * Realtime safe.  This is to be called at the beginning of a process cycle to
 * remove the port from the lists read by the audio thread, so the port
 * will no longer be used and can be removed afterwards.
 *
 * It is the callers responsibility to delete the returned port.
 */
Raul::List<DriverPort*>::Node*
JackMidiDriver::remove_port(const Path& path)
{
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);

	for (Raul::List<JackMidiPort*>::iterator i = _ports.begin(); i != _ports.end(); ++i)
		if ((*i)->patch_port()->path() == path)
			return (Raul::List<DriverPort*>::Node*)_ports.erase(i);

	cerr << "[JackMidiDriver::remove_port] WARNING: Unable to find port " << path << endl;
	return NULL;
}


DriverPort*
JackMidiDriver::driver_port(const Path& path)
{
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);

	for (Raul::List<JackMidiPort*>::iterator i = _ports.begin(); i != _ports.end(); ++i)
		if ((*i)->patch_port()->path() == path)
			return (*i);

	return NULL;
}


} // namespace Ingen

