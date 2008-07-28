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

#include <sstream>
#include <lv2ext/lv2_event.h>
#include "Responder.hpp"
#include "SetPortValueEvent.hpp"
#include "Engine.hpp"
#include "PortImpl.hpp"
#include "ClientBroadcaster.hpp"
#include "NodeImpl.hpp"
#include "ObjectStore.hpp"
#include "AudioBuffer.hpp"
#include "EventBuffer.hpp"
#include "ProcessContext.hpp"

using namespace std;

namespace Ingen {


/** Omni (all voices) control setting */
SetPortValueEvent::SetPortValueEvent(Engine&              engine,
                                     SharedPtr<Responder> responder,
                                     SampleCount          timestamp,
                                     const string&        port_path,
                                     const string&        data_type,
                                     uint32_t             data_size,
                                     const void*          data)
	: Event(engine, responder, timestamp)
	, _omni(true)
	, _voice_num(0)
	, _port_path(port_path)
	, _data_type(data_type)
	, _data_size(data_size)
	, _data(malloc(data_size))
	, _port(NULL)
	, _error(NO_ERROR)
{
	memcpy(_data, data, data_size);
}


/** Voice-specific control setting */
SetPortValueEvent::SetPortValueEvent(Engine&              engine,
                                     SharedPtr<Responder> responder,
                                     SampleCount          timestamp,
                                     uint32_t             voice_num,
                                     const string&        port_path,
                                     const string&        data_type,
                                     uint32_t             data_size,
                                     const void*          data)
	: Event(engine, responder, timestamp)
	, _omni(false)
	, _voice_num(voice_num)
	, _data_type(data_type)
	, _data_size(data_size)
	, _data(malloc(data_size))
	, _port(NULL)
	, _error(NO_ERROR)
{
	memcpy(_data, data, data_size);
}


SetPortValueEvent::~SetPortValueEvent()
{
	free(_data);
}


void
SetPortValueEvent::execute(ProcessContext& context)
{
	Event::execute(context);
	assert(_time >= context.start() && _time <= context.end());

	if (_port == NULL)
		_port = _engine.object_store()->find_port(_port_path);

	if (_port == NULL) {
		_error = PORT_NOT_FOUND;
/*	} else if (_port->buffer(0)->size() < _data_size) {
		_error = NO_SPACE;*/
	} else {
		Buffer* const buf = _port->buffer(0);
		AudioBuffer* const abuf = dynamic_cast<AudioBuffer*>(buf);
		if (abuf) {
			const uint32_t offset = (buf->size() == 1) ? 0 : _time - context.start();

			if (_omni)
				for (uint32_t i=0; i < _port->poly(); ++i)
					((AudioBuffer*)_port->buffer(i))->set(*(float*)_data, offset);
			else
				((AudioBuffer*)_port->buffer(_voice_num))->set(*(float*)_data, offset);

			return;
		}
		
		EventBuffer* const ebuf = dynamic_cast<EventBuffer*>(buf);
		// FIXME: eliminate string comparisons
		if (ebuf && _data_type == "lv2_midi:MidiEvent") {
			const LV2Features::Feature* f = _engine.world()->lv2_features->feature(LV2_URI_MAP_URI);
			LV2URIMap* map = (LV2URIMap*)f->controller;
			const uint32_t type_id = map->uri_to_id(NULL, "http://lv2plug.in/ns/ext/midi#MidiEvent");
			const uint32_t frames = std::max((uint32_t)(_time - context.start()), ebuf->latest_frames());
			ebuf->prepare_write(context.nframes());
			// FIXME: how should this work? binary over OSC, ick
			// Message is an event:
			ebuf->append(frames, 0, type_id, _data_size, (const unsigned char*)_data);
			// Message is an event buffer:
			//ebuf->append((LV2_Event_Buffer*)_data);
			_port->raise_set_by_user_flag();
			return;
		}

		cerr << "WARNING: Unknown value type " << _data_type << ", ignoring" << endl;
	}
}


void
SetPortValueEvent::post_process()
{
	if (_error == NO_ERROR) {
		assert(_port != NULL);
		
		_responder->respond_ok();
		_engine.broadcaster()->send_control_change(_port_path, *(float*)_data);
		
	} else if (_error == PORT_NOT_FOUND) {
		string msg = "Unable to find port ";
		msg.append(_port_path).append(" for set_port_value");
		_responder->respond_error(msg);
	
	} else if (_error == NO_SPACE) {
		std::ostringstream msg("Attempt to write ");
		msg << _data_size << " bytes to " << _port_path << ", with capacity "
			<< _port->buffer_size() << endl;
		_responder->respond_error(msg.str());
	}
}


} // namespace Ingen

