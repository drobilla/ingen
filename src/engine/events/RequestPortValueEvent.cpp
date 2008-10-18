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

#include <string>
#include "RequestPortValueEvent.hpp"
#include "interface/ClientInterface.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "PortImpl.hpp"
#include "EngineStore.hpp"
#include "ClientBroadcaster.hpp"
#include "AudioBuffer.hpp"
#include "ProcessContext.hpp"

using std::string;

namespace Ingen {


RequestPortValueEvent::RequestPortValueEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& port_path)
: QueuedEvent(engine, responder, timestamp),
  _port_path(port_path),
  _port(NULL),
  _value(0.0)
{
}


void
RequestPortValueEvent::pre_process()
{
	_port = _engine.engine_store()->find_port(_port_path);

	QueuedEvent::pre_process();
}


void
RequestPortValueEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);
	assert(_time >= context.start() && _time <= context.end());

	if (_port != NULL && (_port->type() == DataType::CONTROL || _port->type() == DataType::AUDIO))
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
	} else if (_responder->client()) {
		_responder->respond_ok();
		_responder->client()->set_port_value(_port_path, _value);
	} else {
		_responder->respond_error("Unable to find client to send port value");
	}
}


} // namespace Ingen

