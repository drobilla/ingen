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
#include "interface/ClientInterface.hpp"
#include "events/DisablePortMonitoringEvent.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "PortImpl.hpp"
#include "EngineStore.hpp"
#include "ClientBroadcaster.hpp"
#include "AudioBuffer.hpp"

using std::string;

namespace Ingen {


DisablePortMonitoringEvent::DisablePortMonitoringEvent(Engine&              engine,
                                                     SharedPtr<Responder> responder,
                                                     SampleCount          timestamp,
                                                     const std::string&   port_path)
: QueuedEvent(engine, responder, timestamp),
  _port_path(port_path),
  _port(NULL)
{
}


void
DisablePortMonitoringEvent::pre_process()
{
	_port = _engine.object_store()->find_port(_port_path);

	QueuedEvent::pre_process();
}


void
DisablePortMonitoringEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

#if 0
	assert(_time >= start && _time <= end);

	if (_port != NULL && _port->type() == DataType::FLOAT)
		_value = ((AudioBuffer*)_port->buffer(0))->value_at(0/*_time - start*/);
	else 
		_port = NULL; // triggers error response
#endif
}


void
DisablePortMonitoringEvent::post_process()
{
#if 0
	string msg;
	if (!_port) {
		_responder->respond_error("Unable to find port for get_value responder.");
	} else if (_responder->client()) {
		_responder->respond_ok();
		_responder->client()->control_change(_port_path, _value);
	} else {
		_responder->respond_error("Unable to find client to send port value");
	}
#endif
}


} // namespace Ingen

