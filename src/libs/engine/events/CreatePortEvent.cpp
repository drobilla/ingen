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

#include <raul/Path.hpp>
#include <raul/Array.hpp>
#include <raul/List.hpp>
#include <raul/Maid.hpp>
#include "Responder.hpp"
#include "CreatePortEvent.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "Engine.hpp"
#include "PatchImpl.hpp"
#include "QueuedEventSource.hpp"
#include "ObjectStore.hpp"
#include "ClientBroadcaster.hpp"
#include "PortImpl.hpp"
#include "AudioDriver.hpp"
#include "MidiDriver.hpp"
#include "OSCDriver.hpp"
#include "DuplexPort.hpp"

namespace Ingen {


CreatePortEvent::CreatePortEvent(Engine&              engine,
                           SharedPtr<Responder> responder,
                           SampleCount          timestamp,
                           const string&        path,
                           const string&        type,
                           bool                 is_output,
                           QueuedEventSource*   source)
: QueuedEvent(engine, responder, timestamp, true, source),
  _error(NO_ERROR),
  _path(path),
  _type(type),
  _is_output(is_output),
  _data_type(type),
  _patch(NULL),
  _patch_port(NULL),
  _driver_port(NULL)
{
	/* This is blocking because of the two different sets of Patch ports, the array used in the
	 * audio thread (inherited from NodeBase), and the arrays used in the pre processor thread.
	 * If two add port events arrive in the same cycle and the second pre processes before the
	 * first executes, bad things happen (ports are lost).
	 *
	 * FIXME: fix this using RCU
	 */

	if (_data_type == DataType::UNKNOWN) {
		cerr << "[CreatePortEvent] Unknown port type " << type << endl;
		_error = UNKNOWN_TYPE;
	}
}


void
CreatePortEvent::pre_process()
{
	if (_error == UNKNOWN_TYPE || _engine.object_store()->find_object(_path)) {
		QueuedEvent::pre_process();
		return;
	}

	// FIXME: this is just a mess :/
	
	_patch = _engine.object_store()->find_patch(_path.parent());

	if (_patch != NULL) {
		assert(_patch->path() == _path.parent());
		
		size_t buffer_size = 1;
		if (_type != "ingen:Float")
			buffer_size = _engine.audio_driver()->buffer_size();
	
		const uint32_t old_num_ports = _patch->num_ports();

		_patch_port = _patch->create_port(_path.name(), _data_type, buffer_size, _is_output);
		
		if (_patch_port) {

			if (_is_output)
				_patch->add_output(new Raul::List<PortImpl*>::Node(_patch_port));
			else
				_patch->add_input(new Raul::List<PortImpl*>::Node(_patch_port));
			
			if (_patch->external_ports())
				_ports_array = new Raul::Array<PortImpl*>(old_num_ports + 1, *_patch->external_ports());
			else
				_ports_array = new Raul::Array<PortImpl*>(old_num_ports + 1, NULL);


			_ports_array->at(_patch->num_ports()-1) = _patch_port;
			//_patch_port->add_to_store(_engine.object_store());
			_engine.object_store()->add(_patch_port);

			if (!_patch->parent()) {
				if (_type == "ingen:AudioPort")
					_driver_port = _engine.audio_driver()->create_port(
							dynamic_cast<DuplexPort*>(_patch_port));
				else if (_type == "ingen:MIDIPort" || _type == "ingen:EventPort")
					_driver_port = _engine.midi_driver()->create_port(
							dynamic_cast<DuplexPort*>(_patch_port));
				else if (_type == "ingen:OSCPort" && _engine.osc_driver())
					_driver_port = _engine.osc_driver()->create_port(
							dynamic_cast<DuplexPort*>(_patch_port));
			}

			assert(_ports_array->size() == _patch->num_ports());

		}
	}
	QueuedEvent::pre_process();
}


void
CreatePortEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_patch_port) {

		_engine.maid()->push(_patch->external_ports());
		//_patch->add_port(_port);

		_patch->external_ports(_ports_array);
	}

	if (_driver_port) {
		if (_type == "ingen:AudioPort")
			_engine.audio_driver()->add_port(_driver_port);
		else if (_type == "ingen:MIDIPort" || _type == "ingen:EventPort")
			_engine.midi_driver()->add_port(_driver_port);
		else if (_type == "ingen:OSCPort")
			cerr << "OSC DRIVER PORT" << endl;
	}
	
	if (_source)
		_source->unblock();
}


void
CreatePortEvent::post_process()
{
	if (_error != NO_ERROR || !_patch_port) {
		const string msg = string("Could not create port - ").append(_path);
		_responder->respond_error(msg);
	} else {
		_responder->respond_ok();
		_engine.broadcaster()->send_port(_patch_port);
	}
}


} // namespace Ingen

