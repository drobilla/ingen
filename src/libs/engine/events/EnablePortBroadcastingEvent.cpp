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
#include "events/EnablePortBroadcastingEvent.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "Port.hpp"
#include "ObjectStore.hpp"
#include "ClientBroadcaster.hpp"
#include "AudioBuffer.hpp"

using std::string;

namespace Ingen {


EnablePortBroadcastingEvent::EnablePortBroadcastingEvent(Engine&              engine,
                                                         SharedPtr<Responder> responder,
                                                         SampleCount          timestamp,
                                                         const std::string&   port_path,
                                                         bool                 enable)
: QueuedEvent(engine, responder, timestamp),
  _port_path(port_path),
  _port(NULL),
  _enable(enable)
{
}


void
EnablePortBroadcastingEvent::pre_process()
{
	_port = _engine.object_store()->find_port(_port_path);

	QueuedEvent::pre_process();
}


void
EnablePortBroadcastingEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_port)
		_port->monitor(_enable);
}


void
EnablePortBroadcastingEvent::post_process()
{
	if (_port)
		_responder->respond_ok();
	else
		_responder->respond_error("Unable to find port for get_value responder.");
}


} // namespace Ingen

