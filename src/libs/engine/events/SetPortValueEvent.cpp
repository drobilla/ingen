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

#include "SetPortValueEvent.h"
#include "Responder.h"
#include "Engine.h"
#include "TypedPort.h"
#include "ClientBroadcaster.h"
#include "Node.h"
#include "ObjectStore.h"

namespace Ingen {


/** Voice-specific control setting
 */
SetPortValueEvent::SetPortValueEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, size_t voice_num, const string& port_path, Sample val)
: Event(engine, responder, timestamp),
  _voice_num(voice_num),
  _port_path(port_path),
  _val(val),
  _port(NULL),
  _error(NO_ERROR)
{
}


SetPortValueEvent::SetPortValueEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& port_path, Sample val)
: Event(engine, responder, timestamp),
  _voice_num(-1),
  _port_path(port_path),
  _val(val),
  _port(NULL),
  _error(NO_ERROR)
{
}


void
SetPortValueEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	Event::execute(nframes, start, end);
	assert(_time >= start && _time <= end);

	if (_port == NULL)
		_port = _engine.object_store()->find_port(_port_path);

	if (_port == NULL) {
		_error = PORT_NOT_FOUND;
	} else if (!(_port->type() == DataType::FLOAT)) {
		_error = TYPE_MISMATCH;
	} else {
		if (_voice_num == -1) 
			((TypedPort<Sample>*)_port)->set_value(_val, _time - start);
		else
			((TypedPort<Sample>*)_port)->set_value(_voice_num, _val, _time - start);
			//((TypedPort<Sample>*)_port)->buffer(_voice_num)->set(_val, offset); // FIXME: check range
	}
}


void
SetPortValueEvent::post_process()
{
	if (_error == NO_ERROR) {
		assert(_port != NULL);
		
		_responder->respond_ok();
		_engine.broadcaster()->send_control_change(_port_path, _val);
		
	} else if (_error == PORT_NOT_FOUND) {
		string msg = "Unable to find port ";
		msg.append(_port_path).append(" for set_port_value");
		_responder->respond_error(msg);
	
	} else if (_error == TYPE_MISMATCH) {
		string msg = "Attempt to set ";
		msg.append(_port_path).append(" to incompatible type");
		_responder->respond_error(msg);
	}
}


} // namespace Ingen

