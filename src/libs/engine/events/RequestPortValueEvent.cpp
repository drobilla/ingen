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

#include "RequestPortValueEvent.h"
#include <string>
#include "Responder.h"
#include "Engine.h"
#include "interface/ClientInterface.h"
#include "TypedPort.h"
#include "ObjectStore.h"
#include "ClientBroadcaster.h"

using std::string;

namespace Ingen {


RequestPortValueEvent::RequestPortValueEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& port_path)
: QueuedEvent(engine, responder, timestamp),
  m_port_path(port_path),
  m_port(NULL),
  m_value(0.0)
{
}


void
RequestPortValueEvent::pre_process()
{
	m_client = _engine.broadcaster()->client(_responder->client_key());
	m_port = _engine.object_store()->find_port(m_port_path);

	QueuedEvent::pre_process();
}


void
RequestPortValueEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);
	assert(_time >= start && _time <= end);

	if (m_port != NULL && m_port->type() == DataType::FLOAT)
		m_value = ((TypedPort<Sample>*)m_port)->buffer(0)->value_at(_time - start);
	else 
		m_port = NULL; // triggers error response
}


void
RequestPortValueEvent::post_process()
{
	string msg;
	if (m_port) {
		_responder->respond_error("Unable to find port for get_value responder.");
	} else if (m_client) {
		_responder->respond_ok();
		m_client->control_change(m_port_path, m_value);
	} else {
		_responder->respond_error("Unable to find client to send port value");
	}
}


} // namespace Ingen

