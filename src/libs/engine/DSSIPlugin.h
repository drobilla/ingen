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

#ifndef DSSIPLUGIN_H
#define DSSIPLUGIN_H

#include <asoundlib.h>
#include <dssi.h>
#include <lo/lo.h>
#include "LADSPAPlugin.h"

namespace Om {

class MidiMessage;
template <typename T> class InputPort;
namespace Shared {
	class ClientInterface;
} using Shared::ClientInterface;


/** An instance of a DSSI plugin.
 */
class DSSIPlugin : public LADSPAPlugin
{
public:

	typedef map<int, string> Bank;
	
	DSSIPlugin(const string& name, size_t poly, Patch* parent, DSSI_Descriptor* descriptor, samplerate srate, size_t buffer_size);
	~DSSIPlugin();
	
	bool instantiate();

	void activate();

	void set_ui_url(const string& url);
	void send_update();

	void set_control(size_t port_num, sample val);
	void configure(const string& key, const string& val);
	void program(int bank, int program);

	void run(size_t nframes);

	bool update_programs(bool send_events);
	void set_default_program();
	const map<int, Bank>& get_programs() const;

	//void send_creation_messages(ClientInterface* client) const;

	const Plugin* plugin() const  { return m_plugin; }
	void plugin(const Plugin* const pi) { m_plugin = pi; }

private:
	// Prevent copies (undefined)
	DSSIPlugin(const DSSIPlugin& copy);
	DSSIPlugin& operator=(const DSSIPlugin& copy);
	
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
	
	
	DSSI_Descriptor* m_dssi_descriptor;
	
	string     m_ui_url;
	string     m_ui_base_path;
	lo_address m_ui_addr;

	// Current values
	int                 m_bank;
	int                 m_program;
	map<string, string> m_configures;
	map<int, Bank>      m_banks;

	InputPort<MidiMessage>* m_midi_in_port;
 	snd_seq_event_t*        m_alsa_events;
 	unsigned long           m_encoded_events;
 	snd_midi_event_t*       m_alsa_encoder;
};


} // namespace Om


#endif // DSSIPLUGIN_H

