/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef JACKAUDIODRIVER_H
#define JACKAUDIODRIVER_H

#include <jack/jack.h>
#include <jack/transport.h>
#include "List.h"
#include "AudioDriver.h"
#include "Buffer.h"

namespace Om {

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
	JackAudioPort(JackAudioDriver* driver, DuplexPort<sample>* patch_port);
	~JackAudioPort();
	
	void add_to_driver();
	void remove_from_driver();
	void set_name(const string& name) { jack_port_set_name(m_jack_port, name.c_str()); };
	
	void prepare_buffer(jack_nframes_t nframes);

	jack_port_t*          jack_port() const             { return m_jack_port; }
	//DriverBuffer<sample>* buffer() const                { return m_jack_buffer; }
	//void                  jack_buffer(jack_sample_t* s) { m_jack_buffer->set_data(s); }
	DuplexPort<sample>*   patch_port() const            { return m_patch_port; }

private:
	// Prevent copies (undefined)
	JackAudioPort(const JackAudioPort&);
	JackAudioPort& operator=(const JackAudioPort&);
	
	JackAudioDriver*      m_driver;
	jack_port_t*          m_jack_port;
	jack_sample_t*        m_jack_buffer; ///< Cached for output ports
	//DriverBuffer<sample>* m_jack_buffer;
	DuplexPort<sample>*   m_patch_port;
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
	JackAudioDriver();
	JackAudioDriver(jack_client_t *jack_client);
	~JackAudioDriver();

	void activate();
	void deactivate();
	void enable();
	void disable();

	void process_events(jack_nframes_t block_start, jack_nframes_t block_end);
	
	DriverPort* create_port(DuplexPort<sample>* patch_port);
	
	Patch* root_patch()                 { return m_root_patch; }
	void   set_root_patch(Patch* patch) { m_root_patch = patch; }
	
	/** Transport state for this frame.
	 * Intended to only be called from the audio thread. */
	inline const jack_position_t* position() { return &m_position; }
	inline const jack_transport_state_t transport_state() { return m_transport_state; }
	
	bool is_realtime() { return jack_is_realtime(m_client); }
	
	jack_client_t* jack_client() const  { return m_client; }
	samplecount    buffer_size() const  { return m_buffer_size; }
	samplecount    sample_rate() const  { return m_sample_rate; }
	bool           is_activated() const { return m_is_activated; }
	
	samplecount time_stamp() const { return jack_frame_time(m_client); }

private:
	// Prevent copies (undefined)
	JackAudioDriver(const JackAudioDriver&);
	JackAudioDriver& operator=(const JackAudioDriver&);
	
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
	int  m_process_cb(jack_nframes_t nframes);
	void m_shutdown_cb();
	int  m_buffer_size_cb(jack_nframes_t nframes);
	int  m_sample_rate_cb(jack_nframes_t nframes);

	jack_client_t*         m_client;
	jack_nframes_t         m_buffer_size;
	jack_nframes_t         m_sample_rate;
	bool                   m_is_activated;
	bool                   m_local_client; ///< Whether m_client should be closed on destruction
	jack_position_t        m_position;
	jack_transport_state_t m_transport_state;
	
	List<JackAudioPort*> m_ports;

	Patch* m_root_patch;

	jack_nframes_t m_start_of_current_cycle;
	jack_nframes_t m_start_of_last_cycle;
};


inline int JackAudioDriver::process_cb(jack_nframes_t nframes, void* jack_driver)
{
	return ((JackAudioDriver*)jack_driver)->m_process_cb(nframes);
}

inline void JackAudioDriver::shutdown_cb(void* jack_driver)
{
	return ((JackAudioDriver*)jack_driver)->m_shutdown_cb();
}


inline int JackAudioDriver::buffer_size_cb(jack_nframes_t nframes, void* jack_driver)
{
	return ((JackAudioDriver*)jack_driver)->m_buffer_size_cb(nframes);
}


inline int JackAudioDriver::sample_rate_cb(jack_nframes_t nframes, void* jack_driver)
{
	return ((JackAudioDriver*)jack_driver)->m_sample_rate_cb(nframes);
}
	

} // namespace Om

#endif // JACKAUDIODRIVER_H
