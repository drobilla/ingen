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

#ifndef JACKMIDIDRIVER_H
#define JACKMIDIDRIVER_H

#include <jack/jack.h>
#include <jack/midiport.h>
#include "config.h"
#include "List.h"
#include "raul/Queue.h"
#include "MidiDriver.h"

namespace Ingen {

class Node;
class SetPortValueEvent;
class JackMidiDriver;
template <typename T> class DuplexPort;


/** Representation of an JACK MIDI port.
 *
 * \ingroup engine
 */
class JackMidiPort : public DriverPort, public ListNode<JackMidiPort*>
{
public:
	JackMidiPort(JackMidiDriver* driver, DuplexPort<MidiMessage>* port);
	virtual ~JackMidiPort();

	void prepare_block(const SampleCount block_start, const SampleCount block_end);
	
	void add_to_driver();
	void remove_from_driver();
	void set_name(const string& name) { jack_port_set_name(m_jack_port, name.c_str()); };
	
	DuplexPort<MidiMessage>* patch_port() const { return m_patch_port; }

private:
	// Prevent copies (undefined)
	JackMidiPort(const JackMidiPort&);
	JackMidiPort& operator=(const JackMidiPort&);
 
	JackMidiDriver*          m_driver;
	jack_port_t*             m_jack_port;
	DuplexPort<MidiMessage>* m_patch_port;
};


/** Jack MIDI driver.
 *
 * This driver reads Jack MIDI events and dispatches them to the appropriate
 * JackMidiPort for processing.
 *
 * \ingroup engine
 */
class JackMidiDriver : public MidiDriver
{
public:
	JackMidiDriver(jack_client_t* client);
	~JackMidiDriver();

	void activate();
	void deactivate();
	void enable()  { m_is_enabled = true; }
	void disable() { m_is_enabled = false; }
	
	bool is_activated() const { return m_is_activated; }
	bool is_enabled() const   { return m_is_enabled; }
	
	void prepare_block(const SampleCount block_start, const SampleCount block_end);

	JackMidiPort* create_port(DuplexPort<MidiMessage>* patch_port)
	{ return new JackMidiPort(this, patch_port); }

	jack_client_t* jack_client()        { return m_client; }

private:
	// Prevent copies (undefined)
	JackMidiDriver(const JackMidiDriver&);
	JackMidiDriver& operator=(const JackMidiDriver&);
	
	List<JackMidiPort*> m_in_ports;
	List<JackMidiPort*> m_out_ports;
	
	friend class JackMidiPort;
	
	// Functions for JackMidiPort
	void          add_port(JackMidiPort* port);
	JackMidiPort* remove_port(JackMidiPort* port);
	
	void add_output(ListNode<JackMidiPort*>* port);
	ListNode<JackMidiPort*>* remove_output(JackMidiPort* port);
	
	// MIDI thread
	static void* process_midi_in(void* me);

	jack_client_t* m_client;
	pthread_t      m_process_thread;
	bool           m_is_activated;
	bool           m_is_enabled;
	static bool    m_midi_thread_exit_flag;
};


} // namespace Ingen


#endif // JACKMIDIDRIVER_H
