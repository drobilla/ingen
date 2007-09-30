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

#include <map>
//#include <set>
#include "DSSINode.hpp"
#include "ClientBroadcaster.hpp"
#include "interface/ClientInterface.hpp"
#include "InputPort.hpp"
#include "types.hpp"
#include "AudioBuffer.hpp"
#include "ProcessContext.hpp"

using namespace std;

namespace Ingen {


DSSINode::DSSINode(const Plugin* plugin, const string& name, bool polyphonic, Patch* parent, DSSI_Descriptor* descriptor, SampleRate srate, size_t buffer_size)
: LADSPANode(plugin, name, 1, parent, descriptor->LADSPA_Plugin, srate, buffer_size),
  _dssi_descriptor(descriptor),
  _ui_addr(NULL),
  _bank(-1),
  _program(-1),
  _midi_in_port(NULL),
  _midi_in_buffer(NULL),
  _alsa_events(new snd_seq_event_t[_buffer_size]),
  _alsa_encoder(NULL)
{
	snd_midi_event_new(3, &_alsa_encoder);
}


DSSINode::~DSSINode()
{
	if (_ui_addr != NULL)
		lo_address_free(_ui_addr);

	snd_midi_event_free(_alsa_encoder);
	delete [] _alsa_events;
}


/** This needs to be overridden here because LADSPANode::instantiate()
 *  allocates the port array, and we want to add the MIDI input port to that
 *  array.
 */
bool
DSSINode::instantiate()
{
	assert(!_ports);
	
	if (has_midi_input()) {
		_ports = new Raul::Array<Port*>(_descriptor->PortCount + 1);
		_midi_in_port = new InputPort(this, "MIDIIn", _ports->size()-1, 1, DataType::MIDI, _buffer_size);
		_ports->at(_ports->size()-1) = _midi_in_port;
	}	

	// LADSPANode::instantiate checks if _ports is already allocated
	if (!LADSPANode::instantiate()) {
		delete _ports;
		return false;
	}
	
	return true;
}


void
DSSINode::activate()
{
	LADSPANode::activate();
	
	update_programs(false);
	set_default_program();
	
	snd_midi_event_reset_encode(_alsa_encoder);
}


void
DSSINode::set_ui_url(const string& url)
{
	if (_ui_addr != NULL)
		lo_address_free(_ui_addr);
	
	_ui_url = url;
	_ui_addr = lo_address_new_from_url(url.c_str());
	char* base_path = lo_url_get_path(url.c_str());
	_ui_base_path = base_path;
	free(base_path);
	cerr << "Set UI base path to " << _ui_base_path << endl;
}


void
DSSINode::set_control(uint32_t port_num, Sample val)
{
	assert(port_num < _descriptor->PortCount);
	Port* port = _ports->at(port_num);
	assert(port->type() == DataType::FLOAT);
	((AudioBuffer*)port->buffer(0))->set(val, 0);
}


void
DSSINode::configure(const string& key, const string& val)
{
	_dssi_descriptor->configure(_instances[0], key.c_str(), val.c_str());
	_configures[key] = val;
	update_programs(true);
}


void
DSSINode::program(int bank, int program)
{
	if (_dssi_descriptor->select_program)
		_dssi_descriptor->select_program(_instances[0], bank, program);

	_bank = bank;
	_program = program;
}


void
DSSINode::convert_events(SampleCount nframes)
{
	assert(has_midi_input());
	  
	_encoded_events = 0;
	MidiBuffer& buffer = *_midi_in_buffer;
	
	if (_midi_in_buffer == 0)
	  return;
	
	buffer.prepare_read(nframes);
	double timestamp;
	uint32_t size;
	unsigned char* data;
	while (buffer.get_event(&timestamp, &size, &data) < nframes &&
	       _encoded_events < _buffer_size) {
	  snd_midi_event_encode(_alsa_encoder, data, size, 
				&_alsa_events[_encoded_events]);
	  _alsa_events[_encoded_events].time.tick = 
	    (snd_seq_tick_time_t)timestamp;
	  if (_alsa_events[_encoded_events].type != SND_SEQ_EVENT_NONE)
	    ++_encoded_events;
	  buffer.increment();
	}
}


bool
DSSINode::has_midi_input() const
{
	return (_dssi_descriptor->run_synth || _dssi_descriptor->run_multiple_synths);
}


void
DSSINode::process(ProcessContext& context)
{
	NodeBase::pre_process(context);

	if (_dssi_descriptor->run_synth) {
		convert_events(context.nframes());
		_dssi_descriptor->run_synth(_instances[0], context.nframes(),
					     _alsa_events, _encoded_events);
	} else if (_dssi_descriptor->run_multiple_synths) {
		convert_events(context.nframes());
		// I hate this stupid function
		snd_seq_event_t* events[1]       = { _alsa_events };
		long unsigned    events_sizes[1] = { _encoded_events };
		_dssi_descriptor->run_multiple_synths(1, _instances, context.nframes(),
			events, events_sizes);
	} else {
		LADSPANode::process(context);
	}
	
	NodeBase::post_process(context);
}


void
DSSINode::set_port_buffer(uint32_t voice, uint32_t port_num, Buffer* buf)
{
	assert(voice < _polyphony);
	
	// Could be a MIDI port after this
	if (port_num < _descriptor->PortCount) {
	        AudioBuffer* audio_buffer = dynamic_cast<AudioBuffer*>(buf);
		assert(audio_buffer);
		_descriptor->connect_port(_instances[voice], port_num,
			audio_buffer->data());
	}
	
	else if (port_num == _descriptor->PortCount && has_midi_input()) {
	        MidiBuffer* midi_buffer = dynamic_cast<MidiBuffer*>(buf);
		assert(midi_buffer);
		_midi_in_buffer = midi_buffer;
	}
}


void
DSSINode::send_control(int port_num, float value)
{
	string path = _ui_base_path + "/control";
	lo_send(_ui_addr, path.c_str(), "if", port_num, value);
}


void
DSSINode::send_program(int bank, int value)
{
	string path = _ui_base_path + "/program";
	lo_send(_ui_addr, path.c_str(), "ii", bank, value);
}


void
DSSINode::send_configure(const string& key, const string& val)
{
	string path = _ui_base_path + "/configure";
	lo_send(_ui_addr, path.c_str(), "ss", key.c_str(), val.c_str());
}

	
void
DSSINode::send_show()
{
	string path = _ui_base_path + "/show";
	lo_send(_ui_addr, path.c_str(), NULL);
}


void
DSSINode::send_hide()
{
	string path = _ui_base_path + "/hide";
	lo_send(_ui_addr, path.c_str(), NULL);
}


void
DSSINode::send_quit()
{
	string path = _ui_base_path + "/quit";
	lo_send(_ui_addr, path.c_str(), NULL);
}


void
DSSINode::send_update()
{
	// send "configure"s
	for (map<string, string>::iterator i = _configures.begin(); i != _configures.end(); ++i)
		send_configure((*i).first, (*i).second);

	// send "program"
	send_program(_bank, _program);

	// send "control"s
	for (size_t i=0; i < _ports->size(); ++i)
		if (_ports->at(i)->type() == DataType::FLOAT && _ports->at(i)->buffer_size() == 1)
			send_control(_ports->at(i)->num(), ((AudioBuffer*)_ports->at(i)->buffer(0))->value_at(0));

	// send "show" FIXME: not to spec
	send_show();
}


bool
DSSINode::update_programs(bool send_events)
{
#if 0
	// remember all old banks and programs
	set<pair<int, int> > to_be_deleted;
	map<int, Bank>::const_iterator iter;
	Bank::const_iterator iter2;
	for (iter = _banks.begin(); iter != _banks.end(); ++iter) {
	  for (iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2) {
	  	to_be_deleted.insert(make_pair(iter->first, iter2->first));
	  }
	}
  
	// iterate over all programs
	if (_dssi_descriptor->get_program) {
		for (int i = 0; true; ++i) {
			const DSSI_Program_Descriptor* descriptor =
			  _dssi_descriptor->get_program(_instances[0], i);
			if (!descriptor)
			  break;
			
			iter = _banks.find(descriptor->Bank);
			if (iter == _banks.end() || 
			    iter->second.find(descriptor->Program) == iter->second.end() ||
			    iter->second.find(descriptor->Program)->second != descriptor->Name) {
				_banks[descriptor->Bank][descriptor->Program] = descriptor->Name;
				if (send_events) {
					_engine.broadcaster()->send_program_add(path(), descriptor->Bank,
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
		_banks[set_iter->first].erase(set_iter->second);
		if (send_events)
			_engine.broadcaster()->send_program_remove(path(), set_iter->first, set_iter->second);
		if (_banks[set_iter->first].size() == 0)
			_banks.erase(set_iter->first);
	}
#endif
	cerr << "FIXME: DSSI programs broken!" << endl;

	return true;
}


void
DSSINode::set_default_program()
{
	map<int, Bank>::const_iterator iter = _banks.begin();
	if (iter != _banks.end()) {
		Bank::const_iterator iter2 = iter->second.begin();
		if (iter2 != iter->second.end())
			program(iter->first, iter2->first);
	}
}


const map<int, DSSINode::Bank>&
DSSINode::get_programs() const
{
	return _banks;
}


} // namespace Ingen
