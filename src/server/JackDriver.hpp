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

#ifndef INGEN_ENGINE_JACKAUDIODRIVER_HPP
#define INGEN_ENGINE_JACKAUDIODRIVER_HPP

#include "ingen_config.h"

#include <string>
#include <atomic>

#include <jack/jack.h>
#include <jack/transport.h>
#ifdef INGEN_JACK_SESSION
#include <jack/session.h>
#endif

#include "raul/Semaphore.hpp"

#include "lv2/lv2plug.in/ns/ext/atom/forge.h"

#include "Driver.hpp"
#include "EnginePort.hpp"

namespace Raul { class Path; }

namespace Ingen {
namespace Server {

class DuplexPort;
class Engine;
class GraphImpl;
class JackDriver;
class PortImpl;

/** The Jack Driver.
 *
 * The process callback here drives the entire audio thread by "pulling"
 * events from queues, processing them, running the graphs, and passing
 * events along to the PostProcessor.
 *
 * \ingroup engine
 */
class JackDriver : public Driver
{
public:
	explicit JackDriver(Engine& engine);
	~JackDriver();

	bool attach(const std::string& server_name,
	            const std::string& client_name,
	            void*              jack_client);

	void activate();
	void deactivate();
	void enable();
	void disable();

	EnginePort* create_port(DuplexPort* graph_port);
	EnginePort* get_port(const Raul::Path& path);

	void rename_port(const Raul::Path& old_path, const Raul::Path& new_path);
	void port_property(const Raul::Path& path, const Raul::URI& uri, const Atom& value);
	void add_port(ProcessContext& context, EnginePort* port);
	void remove_port(ProcessContext& context, EnginePort* port);
	void register_port(EnginePort& port);
	void unregister_port(EnginePort& port);

	/** Transport state for this frame.
	 * Intended to only be called from the audio thread. */
	inline const jack_position_t* position()        { return &_position; }
	inline jack_transport_state_t transport_state() { return _transport_state; }

	void append_time_events(ProcessContext& context,
	                        Buffer&         buffer);

	jack_client_t* jack_client()  const { return _client; }
	SampleCount    block_length() const { return _block_length; }
	size_t         seq_size()     const { return _seq_size; }
	SampleCount    sample_rate()  const { return _sample_rate; }

	inline SampleCount frame_time() const { return _client ? jack_frame_time(_client) : 0; }

	class PortRegistrationFailedException : public std::exception {};

private:
	friend class JackPort;

	// Static JACK callbacks which call the non-static callbacks (methods)
	inline static void thread_init_cb(void* const jack_driver) {
		return ((JackDriver*)jack_driver)->_thread_init_cb();
	}
	inline static void shutdown_cb(void* const jack_driver) {
		return ((JackDriver*)jack_driver)->_shutdown_cb();
	}
	inline static int process_cb(jack_nframes_t nframes, void* const jack_driver) {
		return ((JackDriver*)jack_driver)->_process_cb(nframes);
	}
	inline static int block_length_cb(jack_nframes_t nframes, void* const jack_driver) {
		return ((JackDriver*)jack_driver)->_block_length_cb(nframes);
	}
#ifdef INGEN_JACK_SESSION
	inline static void session_cb(jack_session_event_t* event, void* jack_driver) {
		((JackDriver*)jack_driver)->_session_cb(event);
	}
#endif

	void pre_process_port(ProcessContext& context, EnginePort* port);
	void post_process_port(ProcessContext& context, EnginePort* port);

	void port_property_internal(const jack_port_t* jport,
	                            const Raul::URI&   uri,
	                            const Atom&        value);

	// Non static callbacks (methods)
	void _thread_init_cb();
	void _shutdown_cb();
	int  _process_cb(jack_nframes_t nframes);
	int  _block_length_cb(jack_nframes_t nframes);
#ifdef INGEN_JACK_SESSION
	void _session_cb(jack_session_event_t* event);
#endif

protected:
	typedef boost::intrusive::list<EnginePort> Ports;

	Engine&                _engine;
	Ports                  _ports;
	LV2_Atom_Forge         _forge;
	Raul::Semaphore        _sem;
	std::atomic<bool>      _flag;
	jack_client_t*         _client;
	jack_nframes_t         _block_length;
	size_t                 _seq_size;
	jack_nframes_t         _sample_rate;
	uint32_t               _midi_event_type;
	bool                   _is_activated;
	jack_position_t        _position;
	jack_transport_state_t _transport_state;
	float                  _old_bpm;
	jack_nframes_t         _old_frame;
	bool                   _old_rolling;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_JACKAUDIODRIVER_HPP
