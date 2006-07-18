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

#ifndef ALSAMIDIDRIVER_H
#define ALSAMIDIDRIVER_H

#include <alsa/asoundlib.h>
#include "List.h"
#include "util/Queue.h"
#include "MidiDriver.h"

namespace Om {

class Node;
class SetPortValueEvent;
class AlsaMidiDriver;
template <typename T> class DuplexPort;

static const int MAX_MIDI_EVENT_SIZE = 3;


/** Representation of an ALSA MIDI port.
 *
 * \ingroup engine
 */
class AlsaMidiPort : public DriverPort, public ListNode<AlsaMidiPort*>
{
public:
	AlsaMidiPort(AlsaMidiDriver* driver, DuplexPort<MidiMessage>* port);
	virtual ~AlsaMidiPort();

	void event(snd_seq_event_t* const ev);
	
	void prepare_block(const SampleCount block_start, const SampleCount block_end);
	
	void add_to_driver();
	void remove_from_driver();
	void set_name(const string& name);
	
	int                      port_id()    const { return m_port_id; }
	DuplexPort<MidiMessage>* patch_port() const { return m_patch_port; }

private:
	// Prevent copies (undefined)
	AlsaMidiPort(const AlsaMidiPort&);
	AlsaMidiPort& operator=(const AlsaMidiPort&);
 
	AlsaMidiDriver*          m_driver;
	DuplexPort<MidiMessage>* m_patch_port;
	int                      m_port_id;
	unsigned char**          m_midi_pool; ///< Pool of raw MIDI events for MidiMessage::buffer
	Queue<snd_seq_event_t>   m_events;
};


/** Alsa MIDI driver.
 *
 * This driver reads Alsa MIDI events and dispatches them to the appropriate
 * AlsaMidiPort for processing.
 *
 * \ingroup engine
 */
class AlsaMidiDriver : public MidiDriver
{
public:
	AlsaMidiDriver();
	~AlsaMidiDriver();

	void activate();
	void deactivate();
	
	bool is_activated() const { return m_is_activated; }
	
	void prepare_block(const SampleCount block_start, const SampleCount block_end);

	DriverPort* create_port(DuplexPort<MidiMessage>* patch_port)
	{ return new AlsaMidiPort(this, patch_port); }

	snd_seq_t*        seq_handle()  const { return m_seq_handle; }
	snd_midi_event_t* event_coder() const { return m_event_coder; }

private:

	// Prevent copies (undefined)
	AlsaMidiDriver(const AlsaMidiDriver&);
	AlsaMidiDriver& operator=(const AlsaMidiDriver&);
	
	List<AlsaMidiPort*> m_in_ports;
	List<AlsaMidiPort*> m_out_ports;
	
	friend class AlsaMidiPort;
	
	// Functions for AlsaMidiPort
	void          add_port(AlsaMidiPort* port);
	AlsaMidiPort* remove_port(AlsaMidiPort* port);
	
	void add_output(ListNode<AlsaMidiPort*>* port);
	ListNode<AlsaMidiPort*>* remove_output(AlsaMidiPort* port);
	
	// MIDI thread
	static void* process_midi_in(void* me);

	snd_seq_t*        m_seq_handle;
	snd_midi_event_t* m_event_coder;
	pthread_t         m_process_thread;
	bool              m_is_activated;
	static bool       m_midi_thread_exit_flag;
};


} // namespace Om


#endif // ALSAMIDIDRIVER_H
