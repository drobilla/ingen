/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
#include "List.h"
#include "AudioDriver.h"
#include "Buffer.h"

namespace Ingen {

class Engine;
class Patch;
class Port;
template <typename T> class DuplexPort;
class JackAudioDriver;
typedef jack_default_audio_sample_t jack_sample_t;


/** Used internally by JackAudioDriver to represent a Jack port.
 *
 * A Jack port always has a one-to-one association with a Patch port.
 */
class JackAudioPort : public DriverPort, public ListNode<JackAudioPort*>
{
public:
	JackAudioPort(JackAudioDriver* driver, DuplexPort<Sample>* patch_port);
	~JackAudioPort();
	
	void add_to_driver();
	void remove_from_driver();
	void set_name(const std::string& name) { jack_port_set_name(_jack_port, name.c_str()); };
	
	void prepare_buffer(jack_nframes_t nframes);

	jack_port_t*          jack_port() const  { return _jack_port; }
	DuplexPort<Sample>*   patch_port() const { return _patch_port; }

private:
	JackAudioDriver*      _driver;
	jack_port_t*          _jack_port;
	jack_sample_t*        _jack_buffer; ///< Cached for output ports
	DuplexPort<Sample>*   _patch_port;
};



/** The Jack AudioDriver.
 *
 * The process callback here drives the entire audio thread by "pulling"
 * events from queues, processing them, running the patches, and passing
 * events along to the PostProcessor.
 *
 * \ingroup engine
 */
class JackAudioDriver : public AudioDriver
{
public:	
	JackAudioDriver(Engine&        engine,
	                std::string    server_name = "",
	                jack_client_t* jack_client = 0);

	~JackAudioDriver();

	void activate();
	void deactivate();
	void enable();
	void disable();

	DriverPort* create_port(DuplexPort<Sample>* patch_port);
	
	Patch* root_patch()                 { return _root_patch; }
	void   set_root_patch(Patch* patch) { _root_patch = patch; }
	
	/** Transport state for this frame.
	 * Intended to only be called from the audio thread. */
	inline const jack_position_t* position() { return &_position; }
	inline const jack_transport_state_t transport_state() { return _transport_state; }
	
	bool is_realtime() { return jack_is_realtime(_client); }
	
	jack_client_t* jack_client() const  { return _client; }
	SampleCount    buffer_size() const  { return _buffer_size; }
	SampleCount    sample_rate() const  { return _sample_rate; }
	bool           is_activated() const { return _is_activated; }
	
	inline SampleCount frame_time() const { return jack_frame_time(_client); }

private:
	friend class JackAudioPort;
	
	// Functions for JackAudioPort
	void           add_port(JackAudioPort* port);
	JackAudioPort* remove_port(JackAudioPort* port);

	// These are the static versions of the callbacks, they call
	// the non-static ones below
	inline static int  process_cb(jack_nframes_t nframes, void* const jack_driver);
	inline static void shutdown_cb(void* const jack_driver);
	inline static int  buffer_size_cb(jack_nframes_t nframes, void* const jack_driver);
	inline static int  sample_rate_cb(jack_nframes_t nframes, void* const jack_driver);

	// Non static callbacks
	int  _process_cb(jack_nframes_t nframes);
	void _shutdown_cb();
	int  _buffer_size_cb(jack_nframes_t nframes);
	int  _sample_rate_cb(jack_nframes_t nframes);

	Engine&                _engine;
	jack_client_t*         _client;
	jack_nframes_t         _buffer_size;
	jack_nframes_t         _sample_rate;
	bool                   _is_activated;
	bool                   _local_client; ///< Whether _client should be closed on destruction
	jack_position_t        _position;
	jack_transport_state_t _transport_state;
	
	List<JackAudioPort*> _ports;

	Patch* _root_patch;
};


inline int JackAudioDriver::process_cb(jack_nframes_t nframes, void* jack_driver)
{
	assert(jack_driver);
	return ((JackAudioDriver*)jack_driver)->_process_cb(nframes);
}

inline void JackAudioDriver::shutdown_cb(void* jack_driver)
{
	assert(jack_driver);
	return ((JackAudioDriver*)jack_driver)->_shutdown_cb();
}


inline int JackAudioDriver::buffer_size_cb(jack_nframes_t nframes, void* jack_driver)
{
	assert(jack_driver);
	return ((JackAudioDriver*)jack_driver)->_buffer_size_cb(nframes);
}


inline int JackAudioDriver::sample_rate_cb(jack_nframes_t nframes, void* jack_driver)
{
	assert(jack_driver);
	return ((JackAudioDriver*)jack_driver)->_sample_rate_cb(nframes);
}
	

} // namespace Ingen

#endif // JACKAUDIODRIVER_H
