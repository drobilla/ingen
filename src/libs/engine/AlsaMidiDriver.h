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

#ifndef ALSAMIDIDRIVER_H
#define ALSAMIDIDRIVER_H

#include <boost/utility.hpp>
#include <alsa/asoundlib.h>
#include <raul/List.h>
#include <raul/SRSWQueue.h>
#include "MidiDriver.h"


namespace Ingen {

class Node;
class SetPortValueEvent;
class AlsaMidiDriver;
class AudioDriver;
class DuplexPort;

static const int MAX_MIDI_EVENT_SIZE = 3;


/** Representation of an ALSA MIDI port.
 *
 * \ingroup engine
 */
class AlsaMidiPort : public DriverPort, public Raul::ListNode<AlsaMidiPort*>
{
public:
	AlsaMidiPort(AlsaMidiDriver* driver, DuplexPort<MidiBuffer>* port);
	virtual ~AlsaMidiPort();

	void event(snd_seq_event_t* const ev);
	
	void prepare_block(const SampleCount block_start, const SampleCount block_end);
	
	void set_name(const std::string& name);
	
	int                      port_id()    const { return _port_id; }
	DuplexPort<MidiBuffer>* patch_port() const { return _patch_port; }

private:
	AlsaMidiDriver*                  _driver;
	DuplexPort<MidiBuffer>*         _patch_port;
	int                              _port_id;
	unsigned char**                  _midi_pool; ///< Pool of raw MIDI events for MidiMessage::buffer
	Raul::SRSWQueue<snd_seq_event_t> _events;
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
	AlsaMidiDriver(AudioDriver* audio_driver);
	~AlsaMidiDriver();

	void activate();
	void deactivate();
	
	bool is_activated() const { return _is_activated; }
	
	void prepare_block(const SampleCount block_start, const SampleCount block_end);

	AudioDriver* audio_driver() { return _audio_driver; }

	DriverPort* create_port(DuplexPort<MidiBuffer>* patch_port)
	{ return new AlsaMidiPort(this, patch_port); }
	
	void        add_port(DriverPort* port);
	DriverPort* remove_port(const Raul::Path& path);

	snd_seq_t*        seq_handle()  const { return _seq_handle; }
	snd_midi_event_t* event_coder() const { return _event_coder; }

private:
	Raul::List<AlsaMidiPort*> _in_ports;
	Raul::List<AlsaMidiPort*> _out_ports;
	
	friend class AlsaMidiPort;
	
	// MIDI thread
	static void* process_midi_in(void* me);

	AudioDriver*      _audio_driver;
	snd_seq_t*        _seq_handle;
	snd_midi_event_t* _event_coder;
	pthread_t         _process_thread;
	bool              _is_activated;
	static bool       _midi_thread_exit_flag;
};


} // namespace Ingen


#endif // ALSAMIDIDRIVER_H
