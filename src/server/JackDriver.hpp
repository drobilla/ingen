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

#include <list>
#include <string>

#include <jack/jack.h>
#include <jack/transport.h>
#ifdef INGEN_JACK_SESSION
#include <jack/session.h>
#endif

#include "raul/AtomicInt.hpp"
#include "raul/List.hpp"
#include "raul/Semaphore.hpp"
#include "raul/Thread.hpp"

#include "Buffer.hpp"
#include "Driver.hpp"
#include "ProcessContext.hpp"

namespace Raul { class Path; }

namespace Ingen {
namespace Server {

class Engine;
class PatchImpl;
class PortImpl;
class DuplexPort;
class JackDriver;

/** Used internally by JackDriver to represent a Jack port.
 *
 * A Jack port always has a one-to-one association with a Patch port.
 */
class JackPort : public DriverPort, public Raul::List<JackPort*>::Node
{
public:
	JackPort(JackDriver* driver, DuplexPort* patch_port);
	~JackPort();

	void create();
	void destroy();

	void move(const Raul::Path& path);

	void pre_process(ProcessContext& context);
	void post_process(ProcessContext& context);

	jack_port_t* jack_port() const { return _jack_port; }

private:
	JackDriver*  _driver;
	jack_port_t* _jack_port;
};

/** The Jack Driver.
 *
 * The process callback here drives the entire audio thread by "pulling"
 * events from queues, processing them, running the patches, and passing
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

	DriverPort* port(const Raul::Path& path);
	DriverPort* create_port(DuplexPort* patch_port);

	void        add_port(DriverPort* port);
	DriverPort* driver_port(const Raul::Path& path);

	Raul::Deletable* remove_port(const Raul::Path& path, DriverPort** port=NULL);

	PatchImpl* root_patch()                     { return _root_patch; }
	void       set_root_patch(PatchImpl* patch) { _root_patch = patch; }

	ProcessContext& context() { return _process_context; }

	/** Transport state for this frame.
	 * Intended to only be called from the audio thread. */
	inline const jack_position_t* position()        { return &_position; }
	inline jack_transport_state_t transport_state() { return _transport_state; }

	bool is_realtime() const { return jack_is_realtime(_client); }

	jack_client_t* jack_client()  const { return _client; }
	SampleCount    block_length() const { return _block_length; }
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
	inline static int sample_rate_cb(jack_nframes_t nframes, void* const jack_driver) {
		return ((JackDriver*)jack_driver)->_sample_rate_cb(nframes);
	}
#ifdef INGEN_JACK_SESSION
	inline static void session_cb(jack_session_event_t* event, void* jack_driver) {
		((JackDriver*)jack_driver)->_session_cb(event);
	}
#endif

	// Non static callbacks (methods)
	void _thread_init_cb();
	void _shutdown_cb();
	int  _process_cb(jack_nframes_t nframes);
	int  _block_length_cb(jack_nframes_t nframes);
	int  _sample_rate_cb(jack_nframes_t nframes);
#ifdef INGEN_JACK_SESSION
	void _session_cb(jack_session_event_t* event);
#endif

	Engine&                              _engine;
	std::list< SharedPtr<Raul::Thread> > _jack_threads;
	Raul::Semaphore                      _sem;
	Raul::AtomicInt                      _flag;
	jack_client_t*                       _client;
	jack_nframes_t                       _block_length;
	jack_nframes_t                       _sample_rate;
	uint32_t                             _midi_event_type;
	bool                                 _is_activated;
	jack_position_t                      _position;
	jack_transport_state_t               _transport_state;
	Raul::List<JackPort*>                _ports;
	ProcessContext                       _process_context;
	PatchImpl*                           _root_patch;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_JACKAUDIODRIVER_HPP
