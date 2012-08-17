/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen_config.h"

#include <cstdlib>
#include <string>

#include <jack/midiport.h>
#ifdef INGEN_JACK_SESSION
#include <jack/session.h>
#include <boost/format.hpp>
#include "ingen/serialisation/Serialiser.hpp"
#endif

#include "ingen/Configuration.hpp"
#include "ingen/LV2Features.hpp"
#include "ingen/Log.hpp"
#include "ingen/World.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"

#include "Buffer.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "JackDriver.hpp"
#include "PatchImpl.hpp"
#include "PortImpl.hpp"
#include "ThreadManager.hpp"
#include "util.hpp"

using namespace std;

typedef jack_default_audio_sample_t jack_sample_t;

namespace Ingen {
namespace Server {

JackDriver::JackDriver(Engine& engine)
	: _engine(engine)
	, _sem(0)
	, _flag(0)
	, _client(NULL)
	, _block_length(0)
	, _sample_rate(0)
	, _is_activated(false)
{
	_midi_event_type = _engine.world()->uris().midi_MidiEvent;
}

JackDriver::~JackDriver()
{
	deactivate();

	if (_client)
		jack_client_close(_client);
}

bool
JackDriver::attach(const std::string& server_name,
                   const std::string& client_name,
                   void*              jack_client)
{
	assert(!_client);
	if (!jack_client) {
#ifdef INGEN_JACK_SESSION
		const std::string uuid = _engine.world()->jack_uuid();
		if (!uuid.empty()) {
			_client = jack_client_open(client_name.c_str(),
			                           JackSessionID, NULL,
			                           uuid.c_str());
			_engine.log().info(Raul::fmt("Connected to JACK as `%1%' (UUID `%2%')\n")
			                % client_name.c_str() % uuid);
		}
#endif

		// Try supplied server name
		if (!_client && !server_name.empty()) {
			if ((_client = jack_client_open(client_name.c_str(),
			                                JackServerName, NULL,
			                                server_name.c_str()))) {
				_engine.log().info(Raul::fmt("Connected to JACK server `%1%'\n")
				                % server_name);
			}
		}

		// Either server name not specified, or supplied server name does not exist
		// Connect to default server
		if (!_client) {
			if ((_client = jack_client_open(client_name.c_str(), JackNullOption, NULL)))
				_engine.log().info("Connected to default JACK server\n");
		}

		// Still failed
		if (!_client) {
			_engine.log().error("Unable to connect to Jack\n");
			return false;
		}
	} else {
		_client = (jack_client_t*)jack_client;
	}

	_block_length = jack_get_buffer_size(_client);
	_sample_rate = jack_get_sample_rate(_client);

	jack_on_shutdown(_client, shutdown_cb, this);

	jack_set_thread_init_callback(_client, thread_init_cb, this);
	jack_set_buffer_size_callback(_client, block_length_cb, this);
#ifdef INGEN_JACK_SESSION
	jack_set_session_callback(_client, session_cb, this);
#endif

	for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i) {
		register_port(*i);
	}

	return true;
}

void
JackDriver::activate()
{
	World* world = _engine.world();

	if (_is_activated) {
		_engine.log().warn("Jack driver already activated\n");
		return;
	}

	if (!_client)
		attach(world->conf().option("jack-server").get_string(),
		       world->conf().option("jack-client").get_string(), NULL);

	jack_set_process_callback(_client, process_cb, this);

	_is_activated = true;

	if (jack_activate(_client)) {
		_engine.log().error("Could not activate Jack client, aborting\n");
		exit(EXIT_FAILURE);
	} else {
		_engine.log().info("Activated Jack client\n");
	}
}

void
JackDriver::deactivate()
{
	if (_is_activated) {
		_flag = 1;
		_is_activated = false;
		_sem.wait();

		for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i) {
			unregister_port(*i);
		}

		if (_client) {
			jack_deactivate(_client);
			jack_client_close(_client);
			_client = NULL;
		}

		_engine.log().info("Deactivated Jack client\n");
	}
}

EnginePort*
JackDriver::get_port(const Raul::Path& path)
{
	for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i) {
		if (i->patch_port()->path() == path) {
			return &*i;
		}
	}

	return NULL;
}

void
JackDriver::add_port(ProcessContext& context, EnginePort* port)
{
	_ports.push_back(*port);
}

void
JackDriver::remove_port(ProcessContext& context, EnginePort* port)
{
	_ports.erase(_ports.iterator_to(*port));
}

void
JackDriver::register_port(EnginePort& port)
{
	jack_port_t* jack_port = jack_port_register(
		_client,
		port.patch_port()->path().substr(1).c_str(),
		(port.patch_port()->is_a(PortType::AUDIO)
		 ? JACK_DEFAULT_AUDIO_TYPE : JACK_DEFAULT_MIDI_TYPE),
		(port.patch_port()->is_input()
		 ? JackPortIsInput : JackPortIsOutput),
		0);

	if (!jack_port) {
		throw JackDriver::PortRegistrationFailedException();
	}

	port.set_handle(jack_port);
}

void
JackDriver::unregister_port(EnginePort& port)
{
	if (jack_port_unregister(_client, (jack_port_t*)port.handle())) {
		_engine.log().error("Failed to unregister Jack port\n");
	}
}

void
JackDriver::rename_port(const Raul::Path& old_path,
                        const Raul::Path& new_path)
{
	EnginePort* eport = get_port(old_path);
	if (eport) {
		jack_port_set_name((jack_port_t*)eport->handle(),
		                   new_path.substr(1).c_str());
	}
}

EnginePort*
JackDriver::create_port(DuplexPort* patch_port)
{
	if (patch_port &&
	    (patch_port->is_a(PortType::AUDIO) ||
	     (patch_port->is_a(PortType::ATOM) &&
	      patch_port->buffer_type() == _engine.world()->uris().atom_Sequence))) {
		EnginePort* eport = new EnginePort(patch_port);
		register_port(*eport);
		patch_port->setup_buffers(*_engine.buffer_factory(),
		                          patch_port->poly(),
		                          false);
		return eport;
	} else {
		return NULL;
	}
}

void
JackDriver::pre_process_port(ProcessContext& context, EnginePort* port)
{
	const SampleCount nframes    = context.nframes();
	jack_port_t*      jack_port  = (jack_port_t*)port->handle();
	PortImpl*         patch_port = port->patch_port();
	void*             buffer     = jack_port_get_buffer(jack_port, nframes);

	port->set_buffer(buffer);

	if (!patch_port->is_input()) {
		patch_port->buffer(0)->clear();
		return;
	}

	if (patch_port->is_a(PortType::AUDIO)) {
		Buffer* patch_buf = patch_port->buffer(0).get();
		memcpy(patch_buf->samples(), buffer, nframes * sizeof(float));

	} else if (patch_port->buffer_type() == patch_port->bufs().uris().atom_Sequence) {
		Buffer* patch_buf = (Buffer*)patch_port->buffer(0).get();

		const jack_nframes_t event_count = jack_midi_get_event_count(buffer);

		patch_buf->prepare_write(context);

		// Copy events from Jack port buffer into patch port buffer
		for (jack_nframes_t i = 0; i < event_count; ++i) {
			jack_midi_event_t ev;
			jack_midi_event_get(&ev, buffer, i);

			if (!patch_buf->append_event(
				    ev.time, ev.size, _midi_event_type, ev.buffer)) {
				_engine.log().warn("Failed to write to MIDI buffer, events lost!\n");
			}
		}
	}
}

void
JackDriver::post_process_port(ProcessContext& context, EnginePort* port)
{
	const SampleCount nframes    = context.nframes();
	jack_port_t*      jack_port  = (jack_port_t*)port->handle();
	PortImpl*         patch_port = port->patch_port();
	void*             buffer     = port->buffer();

	if (patch_port->is_input()) {
		return;
	}

	if (!buffer) {
		// First cycle for a new output, so pre_process wasn't called
		buffer = jack_port_get_buffer(jack_port, nframes);
		port->set_buffer(buffer);
	}

	patch_port->post_process(context);
	Buffer* const patch_buf = patch_port->buffer(0).get();
	if (patch_port->is_a(PortType::AUDIO)) {
		memcpy(buffer, patch_buf->samples(), nframes * sizeof(Sample));
	} else if (patch_port->buffer_type() == patch_port->bufs().uris().atom_Sequence) {
		jack_midi_clear_buffer(buffer);
		LV2_Atom_Sequence* seq = (LV2_Atom_Sequence*)patch_buf->atom();
		LV2_ATOM_SEQUENCE_FOREACH(seq, ev) {
			const uint8_t* buf = (const uint8_t*)LV2_ATOM_BODY(&ev->body);
			if (ev->body.type == patch_port->bufs().uris().midi_MidiEvent) {
				jack_midi_event_write(buffer, ev->time.frames, buf, ev->body.size);
			}
		}
	}
}

/**** Jack Callbacks ****/

/** Jack process callback, drives entire audio thread.
 *
 * \callgraph
 */
REALTIME int
JackDriver::_process_cb(jack_nframes_t nframes)
{
	if (nframes == 0 || ! _is_activated) {
		if (_flag == 1) {
			_sem.post();
		}
		return 0;
	}

	/* Note that Jack can not call this function for a cycle, if overloaded,
	   so a rolling counter here would not always be correct. */
	const jack_nframes_t start_of_current_cycle = jack_last_frame_time(_client);

	_transport_state = jack_transport_query(_client, &_position);

	_engine.process_context().locate(start_of_current_cycle, nframes);

	// Read input
	for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i) {
		pre_process_port(_engine.process_context(), &*i);
	}

	_engine.run(nframes);

	// Write output
	for (Ports::iterator i = _ports.begin(); i != _ports.end(); ++i) {
		post_process_port(_engine.process_context(), &*i);
	}

	return 0;
}

void
JackDriver::_thread_init_cb()
{
	ThreadManager::set_flag(THREAD_PROCESS);
	ThreadManager::set_flag(THREAD_IS_REAL_TIME);
}

void
JackDriver::_shutdown_cb()
{
	_engine.log().info("Jack shutdown, exiting\n");
	_is_activated = false;
	_client = NULL;
}

int
JackDriver::_block_length_cb(jack_nframes_t nframes)
{
	if (_engine.root_patch()) {
		_block_length = nframes;
		_engine.root_patch()->set_buffer_size(
			_engine.process_context(), *_engine.buffer_factory(), PortType::AUDIO,
			_engine.buffer_factory()->audio_buffer_size(nframes));
	}
	return 0;
}

#ifdef INGEN_JACK_SESSION
void
JackDriver::_session_cb(jack_session_event_t* event)
{
	_engine.log().info(Raul::fmt("Jack session save to %1%\n") % event->session_dir);

	const string cmd = (boost::format("ingen -eg -n %1% -u %2% -l ${SESSION_DIR}")
	                    % jack_get_client_name(_client)
	                    % event->client_uuid).str();

	SharedPtr<Serialisation::Serialiser> serialiser = _engine.world()->serialiser();
	if (serialiser) {
		SharedPtr<GraphObject> root(_engine.root_patch(), NullDeleter<GraphObject>);
		serialiser->write_bundle(root, string("file://") + event->session_dir);
	}

	event->command_line = (char*)malloc(cmd.size() + 1);
	memcpy(event->command_line, cmd.c_str(), cmd.size() + 1);
	jack_session_reply(_client, event);

	switch (event->type) {
	case JackSessionSave:
		break;
	case JackSessionSaveAndQuit:
		_engine.log().warn("Jack session quit\n");
		_engine.quit();
		break;
	case JackSessionSaveTemplate:
		break;
	}

	jack_session_event_free(event);
}
#endif

} // namespace Server
} // namespace Ingen
