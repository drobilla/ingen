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

#include "JackAudioDriver.hpp"
#include CONFIG_H_PATH
#include "tuning.hpp"
#include <iostream>
#include <cstdlib>
#include <raul/List.hpp>
#include "Engine.hpp"
#include "util.hpp"
#include "Event.hpp"
#include "ThreadManager.hpp"
#include "QueuedEvent.hpp"
#include "EventSource.hpp"
#include "PostProcessor.hpp"
#include "NodeImpl.hpp"
#include "Patch.hpp"
#include "PortImpl.hpp"
#include "MidiDriver.hpp"
#include "DuplexPort.hpp"
#include "EventSource.hpp"
#include "AudioBuffer.hpp"
#include "ProcessSlave.hpp"
/*#ifdef HAVE_LASH
#include "LashDriver.hpp"
#endif*/

using namespace std;

namespace Ingen {

	
//// JackAudioPort ////

JackAudioPort::JackAudioPort(JackAudioDriver* driver, DuplexPort* patch_port)
: DriverPort(patch_port),
  Raul::ListNode<JackAudioPort*>(this),
  _driver(driver),
  _jack_port(NULL),
  _jack_buffer(NULL)
{
	assert(patch_port->poly() == 1);

	_jack_port = jack_port_register(_driver->jack_client(),
		patch_port->path().c_str(), JACK_DEFAULT_AUDIO_TYPE,
		(patch_port->is_input()) ? JackPortIsInput : JackPortIsOutput,
		0);

	patch_port->buffer(0)->clear();
	patch_port->fixed_buffers(true);
}	


JackAudioPort::~JackAudioPort()
{
	jack_port_unregister(_driver->jack_client(), _jack_port);
}


void
JackAudioPort::prepare_buffer(jack_nframes_t nframes)
{
	jack_sample_t* jack_buf = (jack_sample_t*)jack_port_get_buffer(_jack_port, nframes);

	AudioBuffer* patch_buf = (AudioBuffer*)_patch_port->buffer(0);

	if (jack_buf != _jack_buffer) {
		patch_buf->set_data(jack_buf);
		_jack_buffer = jack_buf;
	}

	assert(patch_buf->data() == jack_buf);
}

	
//// JackAudioDriver ////

JackAudioDriver::JackAudioDriver(Engine&        engine,
                                 std::string    server_name,
                                 jack_client_t* jack_client)
: _engine(engine),
  _jack_thread(NULL),
  _client(jack_client),
  _buffer_size(jack_client ? jack_get_buffer_size(jack_client) : 0),
  _sample_rate(jack_client ? jack_get_sample_rate(jack_client) : 0),
  _is_activated(false),
  _local_client(true), // FIXME
  _process_context(engine),
  _root_patch(NULL)
{
	if (!_client) {
		// Try supplied server name
		if (server_name != "") {
			_client = jack_client_open("Ingen", JackServerName, NULL, server_name.c_str());
			if (_client)
				cerr << "[JackAudioDriver] Connected to JACK server '" <<
					server_name << "'" << endl;
		}
		
		// Either server name not specified, or supplied server name does not exist
		// Connect to default server
		if (!_client) {
			_client = jack_client_open("Ingen", JackNullOption, NULL);
			
			if (_client)
				cerr << "[JackAudioDriver] Connected to default JACK server." << endl;
		}

		// Still failed
		if (!_client) {
			cerr << "[JackAudioDriver] Unable to connect to Jack.  Exiting." << endl;
			exit(EXIT_FAILURE);
		}

		_buffer_size = jack_get_buffer_size(_client);
		_sample_rate = jack_get_sample_rate(_client);
	}

	jack_on_shutdown(_client, shutdown_cb, this);

	jack_set_thread_init_callback(_client, thread_init_cb, this);
	jack_set_sample_rate_callback(_client, sample_rate_cb, this);
	jack_set_buffer_size_callback(_client, buffer_size_cb, this);
}


JackAudioDriver::~JackAudioDriver()
{
	deactivate();
	
	if (_local_client)
		jack_client_close(_client);
}


void
JackAudioDriver::activate()
{	
	if (_is_activated) {
		cerr << "[JackAudioDriver] Jack driver already activated." << endl;
		return;
	}

	jack_set_process_callback(_client, process_cb, this);

	_is_activated = true;

	if (jack_activate(_client)) {
		cerr << "[JackAudioDriver] Could not activate Jack client, aborting." << endl;
		exit(EXIT_FAILURE);
	} else {
		cout << "[JackAudioDriver] Activated Jack client." << endl;
/*#ifdef HAVE_LASH
	_engine.lash_driver()->set_jack_client_name(jack_client_get_name(_client));
#endif*/
	}
}


void
JackAudioDriver::deactivate()
{
	if (_is_activated) {
		
		//for (Raul::List<JackAudioPort*>::iterator i = _ports.begin(); i != _ports.end(); ++i)
		//	jack_port_unregister(_client, (*i)->jack_port());

		jack_deactivate(_client);
		_is_activated = false;
	
		_ports.clear();
	
		cout << "[JackAudioDriver] Deactivated Jack client." << endl;
		
		//_engine.post_processor()->stop();
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
JackAudioDriver::add_port(DriverPort* port)
{
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);

	assert(dynamic_cast<JackAudioPort*>(port));
	_ports.push_back((JackAudioPort*)port);
}


/** Remove a Jack port.
 *
 * Realtime safe.  This is to be called at the beginning of a process cycle to
 * remove the port from the lists read by the audio thread, so the port
 * will no longer be used and can be removed afterwards.
 *
 * It is the callers responsibility to delete the returned port.
 */
DriverPort*
JackAudioDriver::remove_port(const Path& path)
{
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);

	for (Raul::List<JackAudioPort*>::iterator i = _ports.begin(); i != _ports.end(); ++i)
		if ((*i)->patch_port()->path() == path)
			return _ports.erase(i)->elem(); // FIXME: LEAK
	
	cerr << "[JackAudioDriver::remove_port] WARNING: Failed to find Jack port to remove!" << endl;
	return NULL;
}


DriverPort*
JackAudioDriver::port(const Path& path)
{
	for (Raul::List<JackAudioPort*>::iterator i = _ports.begin(); i != _ports.end(); ++i)
		if ((*i)->patch_port()->path() == path)
			return (*i);

	return NULL;
}


DriverPort*
JackAudioDriver::create_port(DuplexPort* patch_port)
{
	if (patch_port->buffer_size() == _buffer_size)
		return new JackAudioPort(this, patch_port);
	else
		return NULL;
}


DriverPort*
JackAudioDriver::driver_port(const Path& path)
{
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);

	for (Raul::List<JackAudioPort*>::iterator i = _ports.begin(); i != _ports.end(); ++i)
		if ((*i)->patch_port()->path() == path)
			return (*i);

	return NULL;
}


/**** Jack Callbacks ****/



/** Jack process callback, drives entire audio thread.
 *
 * \callgraph
 */
int
JackAudioDriver::_process_cb(jack_nframes_t nframes) 
{
	if (nframes == 0)
		return 0;

	// FIXME: all of this time stuff is screwy
	
	// FIXME: support nframes != buffer_size, even though that never damn well happens
	assert(nframes == _buffer_size);

	// Jack can elect to not call this function for a cycle, if overloaded
	// FIXME: this doesn't make sense, and the start time isn't used anyway
	const jack_nframes_t start_of_current_cycle = jack_last_frame_time(_client);
	const jack_nframes_t start_of_last_cycle = start_of_current_cycle - nframes; // FIXME: maybe not..
	const jack_nframes_t end_of_current_cycle = start_of_current_cycle + nframes;

	// FIXME: ditto
	assert(start_of_current_cycle - start_of_last_cycle == nframes);

	_transport_state = jack_transport_query(_client, &_position);

	_process_context.set_time_slice(nframes, start_of_current_cycle, end_of_current_cycle);

	for (Engine::ProcessSlaves::iterator i = _engine.process_slaves().begin();
			i != _engine.process_slaves().end(); ++i) {
		(*i)->context().set_time_slice(nframes, start_of_current_cycle, end_of_current_cycle);
	}
	
	// Process events that came in during the last cycle
	// (Aiming for jitter-free 1 block event latency, ideally)
	if (_engine.event_source())
		_engine.event_source()->process(*_engine.post_processor(), _process_context);
	
	// Set buffers of patch ports to Jack port buffers (zero-copy processing)
	for (Raul::List<JackAudioPort*>::iterator i = _ports.begin(); i != _ports.end(); ++i) {
		assert(*i);
		(*i)->prepare_buffer(nframes);
	}

	assert(_engine.midi_driver());
	_engine.midi_driver()->pre_process(_process_context);
	
	// Run root patch
	if (_root_patch)
		_root_patch->process(_process_context);
	
	_engine.midi_driver()->post_process(_process_context);

	_engine.post_processor()->set_end_time(_process_context.end());

	return 0;
}


void 
JackAudioDriver::_thread_init_cb() 
{
	// Initialize thread specific data
	_jack_thread = Thread::create_for_this_thread("Jack");
	assert(&Thread::get() == _jack_thread);
	_jack_thread->set_context(THREAD_PROCESS);
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);
}

void 
JackAudioDriver::_shutdown_cb() 
{
	cout << "[JackAudioDriver] Jack shutdown.  Exiting." << endl;
	_engine.quit();
	_jack_thread = NULL;
}


int
JackAudioDriver::_sample_rate_cb(jack_nframes_t nframes) 
{
	if (_is_activated) {
		cerr << "[JackAudioDriver] On-the-fly sample rate changing not supported (yet).  Aborting." << endl;
		exit(EXIT_FAILURE);
	} else {
		_sample_rate = nframes;
	}
	return 0;
}


int
JackAudioDriver::_buffer_size_cb(jack_nframes_t nframes) 
{
	if (_root_patch) {
		_root_patch->set_buffer_size(nframes);
		_buffer_size = nframes;
	}
	return 0;
}


} // namespace Ingen

