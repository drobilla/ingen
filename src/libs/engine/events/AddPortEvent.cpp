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

#include "AddPortEvent.h"
#include "Responder.h"
#include "Patch.h"
#include "Tree.h"
#include "Plugin.h"
#include "Engine.h"
#include "Patch.h"
#include "Maid.h"
#include "util/Path.h"
#include "ObjectStore.h"
#include "ClientBroadcaster.h"
#include "util/Path.h"
#include "Port.h"
#include "AudioDriver.h"
#include "MidiDriver.h"
#include "List.h"
#include "Driver.h"
#include "DuplexPort.h"

namespace Ingen {


AddPortEvent::AddPortEvent(CountedPtr<Responder> responder, SampleCount timestamp, const string& path, const string& type, bool is_output)
: QueuedEvent(responder, timestamp),
  _path(path),
  _type(type),
  _is_output(is_output),
  _data_type(DataType::UNKNOWN),
  _patch(NULL),
  _patch_port(NULL),
  _driver_port(NULL)
{
	string type_str;
	if (type == "CONTROL" || type == "AUDIO")
		_data_type = DataType::FLOAT;
	else if (type == "MIDI")
		_data_type = DataType::MIDI;
}


void
AddPortEvent::pre_process()
{
	if (Engine::instance().object_store()->find(_path) != NULL) {
		QueuedEvent::pre_process();
		return;
	}

	// FIXME: this is just a mess :/
	
	_patch = Engine::instance().object_store()->find_patch(_path.parent());

	if (_patch != NULL) {
		assert(_patch->path() == _path.parent());
		
		size_t buffer_size = 1;
		if (_type == "AUDIO" || _type == "MIDI")
			buffer_size = Engine::instance().audio_driver()->buffer_size();
	
		_patch_port = _patch->create_port(_path.name(), _data_type, buffer_size, _is_output);
		if (_patch_port) {
			if (_is_output)
				_patch->add_output(new ListNode<Port*>(_patch_port));
			else
				_patch->add_input(new ListNode<Port*>(_patch_port));
			
			if (_patch->external_ports())
				_ports_array = new Array<Port*>(_patch->num_ports() + 1, *_patch->external_ports());
			else
				_ports_array = new Array<Port*>(_patch->num_ports() + 1, NULL);

			_ports_array->at(_patch->num_ports()) = _patch_port;
			Engine::instance().object_store()->add(_patch_port);

			if (!_patch->parent()) {
				if (_type == "AUDIO")
					_driver_port = Engine::instance().audio_driver()->create_port(
						dynamic_cast<DuplexPort<Sample>*>(_patch_port));
				else if (_type == "MIDI")
					_driver_port = Engine::instance().midi_driver()->create_port(
						dynamic_cast<DuplexPort<MidiMessage>*>(_patch_port));
			}
		}
	}
	QueuedEvent::pre_process();
}


void
AddPortEvent::execute(SampleCount offset)
{
	QueuedEvent::execute(offset);

	if (_patch_port) {
		Engine::instance().maid()->push(_patch->external_ports());
		//_patch->add_port(_port);
		_patch->external_ports(_ports_array);
	}

	if (_driver_port)
		_driver_port->add_to_driver();
}


void
AddPortEvent::post_process()
{
	if (!_patch_port) {
		const string msg = string("Could not create port - ").append(_path);
		_responder->respond_error(msg);
	} else {
		_responder->respond_ok();
		Engine::instance().client_broadcaster()->send_port(_patch_port);
	}
}


} // namespace Ingen

