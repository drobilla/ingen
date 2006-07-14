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

#ifndef DSSINODE_H
#define DSSINODE_H

#include <asoundlib.h>
#include <dssi.h>
#include <lo/lo.h>
#include "LADSPANode.h"

namespace Om {

class MidiMessage;
template <typename T> class InputPort;
namespace Shared {
	class ClientInterface;
} using Shared::ClientInterface;


/** An instance of a DSSI plugin.
 */
class DSSINode : public LADSPANode
{
public:

	typedef map<int, string> Bank;
	
	DSSINode(const Plugin* plugin, const string& name, size_t poly, Patch* parent, DSSI_Descriptor* descriptor, samplerate srate, size_t buffer_size);
	~DSSINode();
	
	bool instantiate();

	void activate();

	void set_ui_url(const string& url);
	void send_update();

	void set_control(size_t port_num, sample val);
	void configure(const string& key, const string& val);
	void program(int bank, int program);

	void process(samplecount nframes);

	bool update_programs(bool send_events);
	void set_default_program();
	const map<int, Bank>& get_programs() const;

	//void send_creation_messages(ClientInterface* client) const;

	const Plugin* plugin() const  { return _plugin; }
	void plugin(const Plugin* const pi) { _plugin = pi; }

private:
	// Prevent copies (undefined)
	DSSINode(const DSSINode& copy);
	DSSINode& operator=(const DSSINode& copy);
	
	bool has_midi_input() const;
	
	// DSSI GUI messages
	void send_control(int port_num, float value);
	void send_program(int bank, int value);
	void send_configure(const string& key, const string& val);
	void send_show();
	void send_hide();
	void send_quit();
	
	// Conversion to ALSA MIDI events
	void convert_events();
	
	
	DSSI_Descriptor* _dssi_descriptor;
	
	string     _ui_url;
	string     _ui_base_path;
	lo_address _ui_addr;

	// Current values
	int                 _bank;
	int                 _program;
	map<string, string> _configures;
	map<int, Bank>      _banks;

	InputPort<MidiMessage>* _midi_in_port;
 	snd_seq_event_t*        _alsa_events;
 	unsigned long           _encoded_events;
 	snd_midi_event_t*       _alsa_encoder;
};


} // namespace Om


#endif // DSSINODE_H

