/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
#include <raul/List.hpp>
#include CONFIG_H_PATH
#include "MidiDriver.hpp"

namespace Ingen {

class Node;
class SetPortValueEvent;
class JackMidiDriver;
class DuplexPort;


/** Representation of an JACK MIDI port.
 *
 * \ingroup engine
 */
class JackMidiPort : public DriverPort, public Raul::ListNode<JackMidiPort*>
{
public:
	JackMidiPort(JackMidiDriver* driver, DuplexPort* port);
	virtual ~JackMidiPort();

	void pre_process(ProcessContext& context);
	void post_process(ProcessContext& context);
	
	void set_name(const std::string& name) { jack_port_set_name(_jack_port, name.c_str()); };

private:
	JackMidiDriver* _driver;
	jack_port_t*    _jack_port;
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
	void enable()  { _is_enabled = true; }
	void disable() { _is_enabled = false; }
	
	bool is_activated() const { return _is_activated; }
	bool is_enabled() const   { return _is_enabled; }
	
	void pre_process(ProcessContext& context);
	void post_process(ProcessContext& context);

	JackMidiPort* create_port(DuplexPort* patch_port)
	{ return new JackMidiPort(this, patch_port); }
	
	void        add_port(DriverPort* port);
	DriverPort* remove_port(const Raul::Path& path);
	
	DriverPort* driver_port(const Raul::Path& path);

	jack_client_t* jack_client()        { return _client; }

private:
	Raul::List<JackMidiPort*> _in_ports;
	Raul::List<JackMidiPort*> _out_ports;
	
	friend class JackMidiPort;
	
	void add_output(Raul::ListNode<JackMidiPort*>* port);
	Raul::ListNode<JackMidiPort*>* remove_output(JackMidiPort* port);
	
	// MIDI thread
	static void* process_midi_in(void* me);

	jack_client_t* _client;
	pthread_t      _process_thread;
	bool           _is_activated;
	bool           _is_enabled;
	static bool    _midi_thread_exit_flag;
};


} // namespace Ingen


#endif // JACKMIDIDRIVER_H
