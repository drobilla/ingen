/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

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

#include "Driver.hpp"
#include "EnginePort.hpp"
#include "ingen_config.h"

#include "ingen/memory.hpp"
#include "lv2/atom/forge.h"
#include "raul/Semaphore.hpp"

#include <jack/jack.h>
#include <jack/thread.h>
#include <jack/transport.h>
#ifdef INGEN_JACK_SESSION
#include <jack/session.h>
#endif

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>

namespace Raul { class Path; }

namespace ingen {
namespace server {

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
	~JackDriver() override;

	bool attach(const std::string& server_name,
	            const std::string& client_name,
	            void*              jack_client);

	bool activate() override;
	void deactivate() override;

	bool dynamic_ports() const override { return true; }

	EnginePort* create_port(DuplexPort* graph_port) override;
	EnginePort* get_port(const Raul::Path& path) override;

	void rename_port(const Raul::Path& old_path, const Raul::Path& new_path) override;
	void port_property(const Raul::Path& path, const URI& uri, const Atom& value) override;
	void add_port(RunContext& ctx, EnginePort* port) override;
	void remove_port(RunContext& ctx, EnginePort* port) override;
	void register_port(EnginePort& port) override;
	void unregister_port(EnginePort& port) override;

	/** Transport state for this frame.
	 * Intended to only be called from the audio thread. */
	inline const jack_position_t* position()        { return &_position; }
	inline jack_transport_state_t transport_state() { return _transport_state; }

	void append_time_events(RunContext& ctx, Buffer& buffer) override;

	int real_time_priority() override {
		return jack_client_real_time_priority(_client);
	}

	jack_client_t* jack_client()  const          { return _client; }
	SampleCount    block_length() const override { return _block_length; }
	size_t         seq_size()     const override { return _seq_size; }
	SampleCount    sample_rate()  const override { return _sample_rate; }

	SampleCount frame_time() const override {
		return _client ? jack_frame_time(_client) : 0;
	}

	class PortRegistrationFailedException : public std::exception {};

private:
	friend class JackPort;

	static void thread_init_cb(void* jack_driver);

	// Static JACK callbacks which call the non-static callbacks (methods)
	inline static void shutdown_cb(void* const jack_driver) {
		return static_cast<JackDriver*>(jack_driver)->_shutdown_cb();
	}
	inline static int process_cb(jack_nframes_t nframes, void* const jack_driver) {
		return static_cast<JackDriver*>(jack_driver)->_process_cb(nframes);
	}
	inline static int block_length_cb(jack_nframes_t nframes, void* const jack_driver) {
		return static_cast<JackDriver*>(jack_driver)->_block_length_cb(nframes);
	}
#ifdef INGEN_JACK_SESSION
	inline static void session_cb(jack_session_event_t* event, void* jack_driver) {
		static_cast<JackDriver*>(jack_driver)->_session_cb(event);
	}
#endif

	void pre_process_port(RunContext& ctx, EnginePort* port);
	void post_process_port(RunContext& ctx, EnginePort* port) const;

	void port_property_internal(const jack_port_t* jport,
	                            const URI&         uri,
	                            const Atom&        value);

	// Non static callbacks (methods)
	void _shutdown_cb();
	int  _process_cb(jack_nframes_t nframes);
	int  _block_length_cb(jack_nframes_t nframes);
#ifdef INGEN_JACK_SESSION
	void _session_cb(jack_session_event_t* event);
#endif

protected:
	using Ports = boost::intrusive::slist<EnginePort,
	                                      boost::intrusive::cache_last<true>>;

	using AudioBufPtr = UPtr<float, FreeDeleter<float>>;

	Engine&                _engine;
	Ports                  _ports;
	AudioBufPtr            _fallback_buffer;
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
	double                 _old_bpm;
	jack_nframes_t         _old_frame;
	bool                   _old_rolling;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_JACKAUDIODRIVER_HPP
