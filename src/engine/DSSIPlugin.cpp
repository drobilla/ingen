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

#include "DSSIPlugin.h"
#include <map>
#include <set>
#include "Om.h"
#include "OmApp.h"
#include "ClientBroadcaster.h"
#include "interface/ClientInterface.h"
#include "InputPort.h"
#include "PortInfo.h"

using namespace std;

namespace Om {


DSSIPlugin::DSSIPlugin(const string& name, size_t poly, Patch* parent, DSSI_Descriptor* descriptor, samplerate srate, size_t buffer_size)
: LADSPAPlugin(name, 1, parent, descriptor->LADSPA_Plugin, srate, buffer_size),
  m_dssi_descriptor(descriptor),
  m_ui_addr(NULL),
  m_bank(-1),
  m_program(-1),
  m_midi_in_port(NULL),
  m_alsa_events(new snd_seq_event_t[m_buffer_size]),
  m_alsa_encoder(NULL)
{
	if (has_midi_input())
		m_num_ports = descriptor->LADSPA_Plugin->PortCount + 1;
	
	snd_midi_event_new(3, &m_alsa_encoder);
}


DSSIPlugin::~DSSIPlugin()
{
	if (m_ui_addr != NULL)
		lo_address_free(m_ui_addr);

	snd_midi_event_free(m_alsa_encoder);
	delete [] m_alsa_events;
}


/** This needs to be overridden here because LADSPAPlugin::instantiate()
 *  allocates the port array, and we want to add the MIDI input port to that
 *  array.
 */
bool
DSSIPlugin::instantiate()
{
	if (!LADSPAPlugin::instantiate())
		return false;
	
	if (has_midi_input()) {
		assert(m_num_ports == m_descriptor->PortCount + 1);
		assert(m_ports.size() == m_descriptor->PortCount + 1);

		m_midi_in_port = new InputPort<MidiMessage>(this, "MIDI In", m_num_ports-1, 1,
				new PortInfo("MIDI In", MIDI, INPUT), m_buffer_size);
		m_ports.at(m_num_ports-1) = m_midi_in_port;
	}	
	
	return true;
}


void
DSSIPlugin::activate()
{
	LADSPAPlugin::activate();
	
	update_programs(false);
	set_default_program();
	
	snd_midi_event_reset_encode(m_alsa_encoder);
}


void
DSSIPlugin::set_ui_url(const string& url)
{
	if (m_ui_addr != NULL)
		lo_address_free(m_ui_addr);
	
	m_ui_url = url;
	m_ui_addr = lo_address_new_from_url(url.c_str());
	char* base_path = lo_url_get_path(url.c_str());
	m_ui_base_path = base_path;
	free(base_path);
	cerr << "Set UI base path to " << m_ui_base_path << endl;
}


void
DSSIPlugin::set_control(size_t port_num, sample val)
{
	assert(port_num < m_descriptor->PortCount);
	((PortBase<sample>*)m_ports.at(port_num))->set_value(val, 0);
}


void
DSSIPlugin::configure(const string& key, const string& val)
{
	m_dssi_descriptor->configure(m_instances[0], key.c_str(), val.c_str());
	m_configures[key] = val;
	update_programs(true);
}


void
DSSIPlugin::program(int bank, int program)
{
	if (m_dssi_descriptor->select_program)
		m_dssi_descriptor->select_program(m_instances[0], bank, program);

	m_bank = bank;
	m_program = program;
}


void
DSSIPlugin::convert_events()
{
	assert(has_midi_input());
	assert(m_midi_in_port != NULL);
	
	Buffer<MidiMessage>& buffer = *m_midi_in_port->buffer(0);
	m_encoded_events = 0;

	for (size_t i = 0; i < buffer.filled_size(); ++i) {
		snd_midi_event_encode(m_alsa_encoder, buffer.value_at(i).buffer,
			buffer.value_at(i).size, 
			&m_alsa_events[m_encoded_events]);
		m_alsa_events[m_encoded_events].time.tick = buffer.value_at(i).time;
		if (m_alsa_events[m_encoded_events].type != SND_SEQ_EVENT_NONE)
			++m_encoded_events;
	}
}


bool
DSSIPlugin::has_midi_input() const
{
	return (m_dssi_descriptor->run_synth || m_dssi_descriptor->run_multiple_synths);
}


void
DSSIPlugin::run(size_t nframes)
{
	NodeBase::run(nframes);

	if (m_dssi_descriptor->run_synth) {
		convert_events();
		m_dssi_descriptor->run_synth(m_instances[0], nframes,
					     m_alsa_events, m_encoded_events);
	} else if (m_dssi_descriptor->run_multiple_synths) {
		convert_events();
		// I hate this stupid function
		snd_seq_event_t* events[1]       = { m_alsa_events };
		long unsigned    events_sizes[1] = { m_encoded_events };
		m_dssi_descriptor->run_multiple_synths(1, m_instances, nframes,
			events, events_sizes);
	} else {
		LADSPAPlugin::run(nframes);
	}
}


void
DSSIPlugin::send_control(int port_num, float value)
{
	string path = m_ui_base_path + "/control";
	lo_send(m_ui_addr, path.c_str(), "if", port_num, value);
}


void
DSSIPlugin::send_program(int bank, int value)
{
	string path = m_ui_base_path + "/program";
	lo_send(m_ui_addr, path.c_str(), "ii", bank, value);
}


void
DSSIPlugin::send_configure(const string& key, const string& val)
{
	string path = m_ui_base_path + "/configure";
	lo_send(m_ui_addr, path.c_str(), "ss", key.c_str(), val.c_str());
}

	
void
DSSIPlugin::send_show()
{
	string path = m_ui_base_path + "/show";
	lo_send(m_ui_addr, path.c_str(), NULL);
}


void
DSSIPlugin::send_hide()
{
	string path = m_ui_base_path + "/hide";
	lo_send(m_ui_addr, path.c_str(), NULL);
}


void
DSSIPlugin::send_quit()
{
	string path = m_ui_base_path + "/quit";
	lo_send(m_ui_addr, path.c_str(), NULL);
}


void
DSSIPlugin::send_update()
{
	// send "configure"s
	for (map<string, string>::iterator i = m_configures.begin(); i != m_configures.end(); ++i)
		send_configure((*i).first, (*i).second);

	// send "program"
	send_program(m_bank, m_program);

	// send "control"s
	for (size_t i=0; i < m_ports.size(); ++i)
		if (m_ports[i]->port_info()->is_control())
			send_control(m_ports[i]->num(), ((PortBase<sample>*)m_ports[i])->buffer(0)->value_at(0));

	// send "show" FIXME: not to spec
	send_show();
}


bool
DSSIPlugin::update_programs(bool send_events)
{
	// remember all old banks and programs
	set<pair<int, int> > to_be_deleted;
	map<int, Bank>::const_iterator iter;
	Bank::const_iterator iter2;
	for (iter = m_banks.begin(); iter != m_banks.end(); ++iter) {
	  for (iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2) {
	  	to_be_deleted.insert(make_pair(iter->first, iter2->first));
	  }
	}
  
	// iterate over all programs
	if (m_dssi_descriptor->get_program) {
		for (int i = 0; true; ++i) {
			const DSSI_Program_Descriptor* descriptor =
			  m_dssi_descriptor->get_program(m_instances[0], i);
			if (!descriptor)
			  break;
			
			iter = m_banks.find(descriptor->Bank);
			if (iter == m_banks.end() || 
			    iter->second.find(descriptor->Program) == iter->second.end() ||
			    iter->second.find(descriptor->Program)->second != descriptor->Name) {
				m_banks[descriptor->Bank][descriptor->Program] = descriptor->Name;
				if (send_events) {
					om->client_broadcaster()->send_program_add(path(), descriptor->Bank,
									   descriptor->Program, 
									   descriptor->Name);
				}
				to_be_deleted.erase(make_pair(descriptor->Bank, descriptor->Program));
			}
		}
	}
	
	// remove programs that has disappeared from the plugin
	set<pair<int, int> >::const_iterator set_iter;
	for (set_iter = to_be_deleted.begin(); 
	     set_iter != to_be_deleted.end(); ++set_iter) {
		m_banks[set_iter->first].erase(set_iter->second);
		if (send_events)
			om->client_broadcaster()->send_program_remove(path(), set_iter->first, set_iter->second);
		if (m_banks[set_iter->first].size() == 0)
			m_banks.erase(set_iter->first);
	}
	
	return true;
}


void
DSSIPlugin::set_default_program()
{
	map<int, Bank>::const_iterator iter = m_banks.begin();
	if (iter != m_banks.end()) {
		Bank::const_iterator iter2 = iter->second.begin();
		if (iter2 != iter->second.end())
			program(iter->first, iter2->first);
	}
}


const map<int, DSSIPlugin::Bank>&
DSSIPlugin::get_programs() const
{
	return m_banks;
}


/*
void
DSSIPlugin::send_creation_messages(ClientInterface* client) const
{
	LADSPAPlugin::send_creation_messages(client);
	
	for (map<int, Bank>::const_iterator i = get_programs().begin();
			i != get_programs().end(); ++i) {
		
		for (Bank::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
		  client->program_add(path(), 
			i->first, j->first, j->second);
	}
}
*/

} // namespace Om
