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

#include "RequestPortValueEvent.h"
#include <string>
#include "interface/ClientInterface.h"
#include "interface/Responder.h"
#include "Engine.h"
#include "Port.h"
#include "ObjectStore.h"
#include "ClientBroadcaster.h"
#include "AudioBuffer.h"

using std::string;

namespace Ingen {


RequestPortValueEvent::RequestPortValueEvent(Engine& engine, SharedPtr<Shared::Responder> responder, SampleCount timestamp, const string& port_path)
: QueuedEvent(engine, responder, timestamp),
  _port_path(port_path),
  _port(NULL),
  _value(0.0)
{
}


void
RequestPortValueEvent::pre_process()
{
	_client = _engine.broadcaster()->client(_responder->client_key());
	_port = _engine.object_store()->find_port(_port_path);

	QueuedEvent::pre_process();
}


void
RequestPortValueEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);
	assert(_time >= start && _time <= end);

	if (_port != NULL && _port->type() == DataType::FLOAT)
		_value = ((AudioBuffer*)_port->buffer(0))->value_at(0/*_time - start*/);
	else 
		_port = NULL; // triggers error response
}


void
RequestPortValueEvent::post_process()
{
	string msg;
	if (!_port) {
		_responder->respond_error("Unable to find port for get_value responder.");
	} else if (_client) {
		_responder->respond_ok();
		_client->control_change(_port_path, _value);
	} else {
		_responder->respond_error("Unable to find client to send port value");
	}
}


} // namespace Ingen

