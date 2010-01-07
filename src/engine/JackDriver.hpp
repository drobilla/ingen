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
 */

#ifndef JACKAUDIODRIVER_H
#define JACKAUDIODRIVER_H

#include <jack/jack.h>
#include <jack/transport.h>
#include "raul/AtomicInt.hpp"
#include "raul/List.hpp"
#include "raul/Semaphore.hpp"
#include "raul/Thread.hpp"
#include "Driver.hpp"
#include "Buffer.hpp"
#include "ProcessContext.hpp"

namespace Raul { class Path; }

namespace Ingen {

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
	JackDriver(Engine& engine);
	~JackDriver();

	bool supports(Shared::PortType port_type, Shared::EventType event_type);

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

	Raul::List<DriverPort*>::Node* remove_port(const Raul::Path& path);

	PatchImpl* root_patch()                     { return _root_patch; }
	void       set_root_patch(PatchImpl* patch) { _root_patch = patch; }

	ProcessContext& context() { return _process_context; }

	/** Transport state for this frame.
	 * Intended to only be called from the audio thread. */
	inline const jack_position_t* position()        { return &_position; }
	inline jack_transport_state_t transport_state() { return _transport_state; }

	bool is_realtime() const { return jack_is_realtime(_client); }

	jack_client_t* jack_client()  const { return _client; }
	SampleCount    buffer_size()  const { return _buffer_size; }
	SampleCount    sample_rate()  const { return _sample_rate; }
	bool           is_activated() const { return _is_activated; }

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
	inline static int buffer_size_cb(jack_nframes_t nframes, void* const jack_driver) {
		return ((JackDriver*)jack_driver)->_buffer_size_cb(nframes);
	}
	inline static int sample_rate_cb(jack_nframes_t nframes, void* const jack_driver) {
		return ((JackDriver*)jack_driver)->_sample_rate_cb(nframes);
	}

	// Non static callbacks (methods)
	void _thread_init_cb();
	void _shutdown_cb();
	int  _process_cb(jack_nframes_t nframes);
	int  _buffer_size_cb(jack_nframes_t nframes);
	int  _sample_rate_cb(jack_nframes_t nframes);

	Engine&                _engine;
	Raul::Thread*          _jack_thread;
	Raul::Semaphore        _sem;
	Raul::AtomicInt        _flag;
	jack_client_t*         _client;
	jack_nframes_t         _buffer_size;
	jack_nframes_t         _sample_rate;
	uint32_t               _midi_event_type;
	bool                   _is_activated;
	bool                   _local_client; ///< Whether _client should be closed on destruction
	jack_position_t        _position;
	jack_transport_state_t _transport_state;
	Raul::List<JackPort*>  _ports;
	ProcessContext         _process_context;
	PatchImpl*             _root_patch;
};


} // namespace Ingen

#endif // JACKAUDIODRIVER_H
