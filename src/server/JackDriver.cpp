/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "JackDriver.hpp"

#include "Buffer.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "PortImpl.hpp"
#include "ThreadManager.hpp"
#include "ingen_config.h"
#include "util.hpp"

#include "ingen/Configuration.hpp"
#include "ingen/LV2Features.hpp"
#include "ingen/Log.hpp"
#include "ingen/URI.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/World.hpp"
#include "ingen/fmt.hpp"
#include "lv2/atom/util.h"

#include <jack/midiport.h>

#ifdef INGEN_JACK_SESSION
#include "ingen/Serialiser.hpp"
#include <jack/session.h>
#endif

#ifdef HAVE_JACK_METADATA
#include "jackey.h"
#include <jack/metadata.h>
#endif

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <utility>

using jack_sample_t = jack_default_audio_sample_t;

namespace ingen {
namespace server {

JackDriver::JackDriver(Engine& engine)
	: _engine(engine)
	, _forge()
	, _sem(0)
	, _flag(false)
	, _client(nullptr)
	, _block_length(0)
	, _seq_size(0)
	, _sample_rate(0)
	, _is_activated(false)
	, _position()
	, _transport_state()
	, _old_bpm(120.0f)
	, _old_frame(0)
	, _old_rolling(false)
{
	_midi_event_type = _engine.world().uris().midi_MidiEvent;
	lv2_atom_forge_init(
		&_forge, &engine.world().uri_map().urid_map_feature()->urid_map);
}

JackDriver::~JackDriver()
{
	deactivate();
	_ports.clear_and_dispose([](EnginePort* p) { delete p; });
}

bool
JackDriver::attach(const std::string& server_name,
                   const std::string& client_name,
                   void*              jack_client)
{
	assert(!_client);
	if (!jack_client) {
#ifdef INGEN_JACK_SESSION
		const std::string uuid = _engine.world().jack_uuid();
		if (!uuid.empty()) {
			_client = jack_client_open(client_name.c_str(),
			                           JackSessionID, nullptr,
			                           uuid.c_str());
			_engine.log().info("Connected to Jack as `%1%' (UUID `%2%')\n",
			                   client_name.c_str(), uuid);
		}
#endif

		// Try supplied server name
		if (!_client && !server_name.empty()) {
			if ((_client = jack_client_open(client_name.c_str(),
			                                JackServerName, nullptr,
			                                server_name.c_str()))) {
				_engine.log().info("Connected to Jack server `%1%'\n", server_name);
			}
		}

		// Either server name not specified, or supplied server name does not exist
		// Connect to default server
		if (!_client) {
			if ((_client = jack_client_open(client_name.c_str(), JackNullOption, nullptr))) {
				_engine.log().info("Connected to default Jack server\n");
			}
		}

		// Still failed
		if (!_client) {
			_engine.log().error("Unable to connect to Jack\n");
			return false;
		}
	} else {
		_client = static_cast<jack_client_t*>(jack_client);
	}

	_sample_rate  = jack_get_sample_rate(_client);
	_block_length = jack_get_buffer_size(_client);
	_seq_size     = jack_port_type_get_buffer_size(_client, JACK_DEFAULT_MIDI_TYPE);

	_fallback_buffer = AudioBufPtr(
		static_cast<float*>(
			Buffer::aligned_alloc(sizeof(float) * _block_length)));

	jack_on_shutdown(_client, shutdown_cb, this);

	jack_set_thread_init_callback(_client, thread_init_cb, this);
	jack_set_buffer_size_callback(_client, block_length_cb, this);
#ifdef INGEN_JACK_SESSION
	jack_set_session_callback(_client, session_cb, this);
#endif

	for (auto& p : _ports) {
		register_port(p);
	}

	return true;
}

bool
JackDriver::activate()
{
	World& world = _engine.world();

	if (_is_activated) {
		_engine.log().warn("Jack driver already activated\n");
		return false;
	}

	if (!_client) {
		attach(world.conf().option("jack-server").ptr<char>(),
		       world.conf().option("jack-name").ptr<char>(), nullptr);
	}

	if (!_client) {
		return false;
	}

	jack_set_process_callback(_client, process_cb, this);

	_is_activated = true;

	if (jack_activate(_client)) {
		_engine.log().error("Could not activate Jack client, aborting\n");
		return false;
	} else {
		_engine.log().info("Activated Jack client `%1%'\n",
		                   world.conf().option("jack-name").ptr<char>());
	}
	return true;
}

void
JackDriver::deactivate()
{
	if (_is_activated) {
		_flag         = true;
		_is_activated = false;
		_sem.timed_wait(std::chrono::seconds(1));

		for (auto& p : _ports) {
			unregister_port(p);
		}

		if (_client) {
			jack_deactivate(_client);
			jack_client_close(_client);
			_client = nullptr;
		}

		_engine.log().info("Deactivated Jack client\n");
	}
}

EnginePort*
JackDriver::get_port(const Raul::Path& path)
{
	for (auto& p : _ports) {
		if (p.graph_port()->path() == path) {
			return &p;
		}
	}

	return nullptr;
}

void
JackDriver::add_port(RunContext& ctx, EnginePort* port)
{
	_ports.push_back(*port);

	DuplexPort* graph_port = port->graph_port();
	if (graph_port->is_a(PortType::AUDIO) || graph_port->is_a(PortType::CV)) {
		const SampleCount nframes = ctx.nframes();
		auto*             jport   = static_cast<jack_port_t*>(port->handle());
		void*             jbuf    = jack_port_get_buffer(jport, nframes);

		/* Jack fails to return a buffer if this is too soon after registering
		   the port, so use a silent fallback buffer if necessary. */
		graph_port->set_driver_buffer(
			jbuf ? jbuf : _fallback_buffer.get(),
			nframes * sizeof(float));
	}
}

void
JackDriver::remove_port(RunContext&, EnginePort* port)
{
	_ports.erase(_ports.iterator_to(*port));
}

void
JackDriver::register_port(EnginePort& port)
{
	jack_port_t* jack_port = jack_port_register(
		_client,
		port.graph_port()->path().substr(1).c_str(),
		((port.graph_port()->is_a(PortType::AUDIO) ||
		  port.graph_port()->is_a(PortType::CV))
		 ? JACK_DEFAULT_AUDIO_TYPE : JACK_DEFAULT_MIDI_TYPE),
		(port.graph_port()->is_input()
		 ? JackPortIsInput : JackPortIsOutput),
		0);

	if (!jack_port) {
		throw JackDriver::PortRegistrationFailedException();
	}

	port.set_handle(jack_port);

	for (const auto& p : port.graph_port()->properties()) {
		port_property_internal(jack_port, p.first, p.second);
	}
}

void
JackDriver::unregister_port(EnginePort& port)
{
	if (jack_port_unregister(_client, static_cast<jack_port_t*>(port.handle()))) {
		_engine.log().error("Failed to unregister Jack port\n");
	}

	port.set_handle(nullptr);
}

void
JackDriver::rename_port(const Raul::Path& old_path,
                        const Raul::Path& new_path)
{
	EnginePort* eport = get_port(old_path);
	if (eport) {
#ifdef HAVE_JACK_PORT_RENAME
		jack_port_rename(_client,
		                 static_cast<jack_port_t*>(eport->handle()),
		                 new_path.substr(1).c_str());
#else
		jack_port_set_name((jack_port_t*)eport->handle(),
		                   new_path.substr(1).c_str());
#endif
	}
}

void
JackDriver::port_property(const Raul::Path& path,
                          const URI&        uri,
                          const Atom&       value)
{
#ifdef HAVE_JACK_METADATA
	EnginePort* eport = get_port(path);
	if (eport) {
		const auto* const jport =
		    static_cast<const jack_port_t*>(eport->handle());

		port_property_internal(jport, uri, value);
	}
#endif
}

void
JackDriver::port_property_internal(const jack_port_t* jport,
                                   const URI&         uri,
                                   const Atom&        value)
{
#ifdef HAVE_JACK_METADATA
	if (uri == _engine.world().uris().lv2_name) {
		jack_set_property(_client, jack_port_uuid(jport),
		                  JACK_METADATA_PRETTY_NAME, value.ptr<char>(), "text/plain");
	} else if (uri == _engine.world().uris().lv2_index) {
		jack_set_property(_client, jack_port_uuid(jport),
		                  JACKEY_ORDER, std::to_string(value.get<int32_t>()).c_str(),
		                  "http://www.w3.org/2001/XMLSchema#integer");
	} else if (uri == _engine.world().uris().rdf_type) {
		if (value == _engine.world().uris().lv2_CVPort) {
			jack_set_property(_client, jack_port_uuid(jport),
			                  JACKEY_SIGNAL_TYPE, "CV", "text/plain");
		}
	}
#endif
}

EnginePort*
JackDriver::create_port(DuplexPort* graph_port)
{
	EnginePort* eport = nullptr;
	if (graph_port->is_a(PortType::AUDIO) || graph_port->is_a(PortType::CV)) {
		// Audio buffer port, use Jack buffer directly
		eport = new EnginePort(graph_port);
		graph_port->set_is_driver_port(*_engine.buffer_factory());
	} else if (graph_port->is_a(PortType::ATOM) &&
	           graph_port->buffer_type() == _engine.world().uris().atom_Sequence) {
		// Sequence port, make Jack port but use internal LV2 format buffer
		eport = new EnginePort(graph_port);
	}

	if (eport) {
		register_port(*eport);
	}

	return eport;
}

void
JackDriver::pre_process_port(RunContext& ctx, EnginePort* port)
{
	const URIs&       uris       = ctx.engine().world().uris();
	const SampleCount nframes    = ctx.nframes();
	auto*             jack_port  = static_cast<jack_port_t*>(port->handle());
	DuplexPort*       graph_port = port->graph_port();
	Buffer*           graph_buf  = graph_port->buffer(0).get();
	void*             jack_buf   = jack_port_get_buffer(jack_port, nframes);

	if (graph_port->is_a(PortType::AUDIO) || graph_port->is_a(PortType::CV)) {
		graph_port->set_driver_buffer(jack_buf, nframes * sizeof(float));
		if (graph_port->is_input()) {
			graph_port->monitor(ctx);
		} else {
			graph_port->buffer(0)->clear(); // TODO: Avoid when possible
		}
	} else if (graph_port->buffer_type() == uris.atom_Sequence) {
		graph_buf->prepare_write(ctx);
		if (graph_port->is_input()) {
			// Copy events from Jack port buffer into graph port buffer
			const jack_nframes_t event_count = jack_midi_get_event_count(jack_buf);
			for (jack_nframes_t i = 0; i < event_count; ++i) {
				jack_midi_event_t ev;
				jack_midi_event_get(&ev, jack_buf, i);
				if (!graph_buf->append_event(
					    ev.time, ev.size, _midi_event_type, ev.buffer)) {
					_engine.log().rt_error("Failed to write to MIDI buffer, events lost!\n");
				}
			}
		}
		graph_port->monitor(ctx);
	}
}

void
JackDriver::post_process_port(RunContext& ctx, EnginePort* port) const
{
	const URIs&       uris       = ctx.engine().world().uris();
	const SampleCount nframes    = ctx.nframes();
	auto*             jack_port  = static_cast<jack_port_t*>(port->handle());
	DuplexPort*       graph_port = port->graph_port();
	void*             jack_buf   = port->buffer();

	if (port->graph_port()->is_output()) {
		if (!jack_buf) {
			// First cycle for a new output, so pre_process wasn't called
			jack_buf = jack_port_get_buffer(jack_port, nframes);
			port->set_buffer(jack_buf);
		}

		if (graph_port->buffer_type() == uris.atom_Sequence) {
			// Copy LV2 MIDI events to Jack MIDI buffer
			Buffer* const graph_buf = graph_port->buffer(0).get();
			auto*         seq       = graph_buf->get<LV2_Atom_Sequence>();

			jack_midi_clear_buffer(jack_buf);
			LV2_ATOM_SEQUENCE_FOREACH(seq, ev) {
				const auto* buf =
				    static_cast<const uint8_t*>(LV2_ATOM_BODY(&ev->body));

				if (ev->body.type == this->_midi_event_type) {
					jack_midi_event_write(
						jack_buf, ev->time.frames, buf, ev->body.size);
				}
			}
		}
	}

	// Reset graph port buffer pointer to no longer point to the Jack buffer
	if (graph_port->is_driver_port()) {
		graph_port->set_driver_buffer(nullptr, 0);
	}
}

void
JackDriver::append_time_events(RunContext& ctx, Buffer& buffer)
{
	const URIs&            uris    = ctx.engine().world().uris();
	const jack_position_t* pos     = &_position;
	const bool             rolling = (_transport_state == JackTransportRolling);

	// Do nothing if there is no unexpected time change (other than rolling)
	if (rolling == _old_rolling &&
	    pos->frame == _old_frame &&
	    pos->beats_per_minute == _old_bpm) {
		return;
	}

	// Update old time information to detect change next cycle
	_old_frame   = pos->frame;
	_old_rolling = rolling;
	_old_bpm     = pos->beats_per_minute;

	// Build an LV2 position object to append to the buffer
	LV2_Atom             pos_buf[16];
	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_set_buffer(&_forge,
	                          reinterpret_cast<uint8_t*>(pos_buf),
	                          sizeof(pos_buf));

	lv2_atom_forge_object(&_forge, &frame, 0, uris.time_Position);
	lv2_atom_forge_key(&_forge, uris.time_frame);
	lv2_atom_forge_long(&_forge, pos->frame);
	lv2_atom_forge_key(&_forge, uris.time_speed);
	lv2_atom_forge_float(&_forge, rolling ? 1.0 : 0.0);
	if (pos->valid & JackPositionBBT) {
		lv2_atom_forge_key(&_forge, uris.time_barBeat);
		lv2_atom_forge_float(
			&_forge, pos->beat - 1 + (pos->tick / pos->ticks_per_beat));
		lv2_atom_forge_key(&_forge, uris.time_bar);
		lv2_atom_forge_long(&_forge, pos->bar - 1);
		lv2_atom_forge_key(&_forge, uris.time_beatUnit);
		lv2_atom_forge_int(&_forge, pos->beat_type);
		lv2_atom_forge_key(&_forge, uris.time_beatsPerBar);
		lv2_atom_forge_float(&_forge, pos->beats_per_bar);
		lv2_atom_forge_key(&_forge, uris.time_beatsPerMinute);
		lv2_atom_forge_float(&_forge, pos->beats_per_minute);
	}

	// Append position to buffer at offset 0 (start of this cycle)
	auto* lpos = static_cast<LV2_Atom*>(pos_buf);
	buffer.append_event(0,
	                    lpos->size,
	                    lpos->type,
	                    static_cast<const uint8_t*>(LV2_ATOM_BODY_CONST(lpos)));
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
		if (_flag) {
			_sem.post();
		}
		return 0;
	}

	/* Note that Jack may not call this function for a cycle, if overloaded,
	   so a rolling counter here would not always be correct. */
	const jack_nframes_t start_of_current_cycle = jack_last_frame_time(_client);

	_transport_state = jack_transport_query(_client, &_position);

	_engine.locate(start_of_current_cycle, nframes);

	// Read input
	for (auto& p : _ports) {
		pre_process_port(_engine.run_context(), &p);
	}

	// Process
	_engine.run(nframes);

	// Write output
	for (auto& p : _ports) {
		post_process_port(_engine.run_context(), &p);
	}

	// Update expected transport frame for next cycle to detect changes
	if (_transport_state == JackTransportRolling) {
		_old_frame += nframes;
	}

	return 0;
}

void
JackDriver::thread_init_cb(void*)
{
	ThreadManager::set_flag(THREAD_PROCESS);
	ThreadManager::set_flag(THREAD_IS_REAL_TIME);
}

void
JackDriver::_shutdown_cb()
{
	_engine.log().info("Jack shutdown, exiting\n");
	_is_activated = false;
	_client = nullptr;
}

int
JackDriver::_block_length_cb(jack_nframes_t nframes)
{
	if (_engine.root_graph()) {
		_block_length = nframes;
		_seq_size = jack_port_type_get_buffer_size(_client, JACK_DEFAULT_MIDI_TYPE);
		_engine.root_graph()->set_buffer_size(
			_engine.run_context(), *_engine.buffer_factory(), PortType::AUDIO,
			_engine.buffer_factory()->audio_buffer_size(nframes));
		_engine.root_graph()->set_buffer_size(
			_engine.run_context(), *_engine.buffer_factory(), PortType::ATOM,
			_seq_size);
	}
	return 0;
}

#ifdef INGEN_JACK_SESSION
void
JackDriver::_session_cb(jack_session_event_t* event)
{
	_engine.log().info("Jack session save to %1%\n", event->session_dir);

	const std::string cmd = fmt("ingen -eg -n %1% -u %2% -l ${SESSION_DIR}",
	                            jack_get_client_name(_client),
	                            event->client_uuid);

	SPtr<Serialiser> serialiser = _engine.world().serialiser();
	if (serialiser) {
		std::lock_guard<std::mutex> lock(_engine.world().rdf_mutex());

		SPtr<Node> root(_engine.root_graph(), NullDeleter<Node>);
		serialiser->write_bundle(root,
		                         URI(std::string("file://") + event->session_dir));
	}

	event->command_line = static_cast<char*>(malloc(cmd.size() + 1));
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

} // namespace server
} // namespace ingen
