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

#include "SetPortValueQueuedEvent.h"
#include "Responder.h"
#include "Engine.h"
#include "TypedPort.h"
#include "ClientBroadcaster.h"
#include "Plugin.h"
#include "Node.h"
#include "ObjectStore.h"

namespace Ingen {


/** Voice-specific control setting
 */
SetPortValueQueuedEvent::SetPortValueQueuedEvent(Engine& engine, CountedPtr<Responder> responder, SampleCount timestamp, size_t voice_num, const string& port_path, Sample val)
: QueuedEvent(engine, responder, timestamp),
  m_voice_num(voice_num),
  m_port_path(port_path),
  m_val(val),
  m_port(NULL),
  m_error(NO_ERROR)
{
}


SetPortValueQueuedEvent::SetPortValueQueuedEvent(Engine& engine, CountedPtr<Responder> responder, SampleCount timestamp, const string& port_path, Sample val)
: QueuedEvent(engine, responder, timestamp),
  m_voice_num(-1),
  m_port_path(port_path),
  m_val(val),
  m_port(NULL),
  m_error(NO_ERROR)
{
}


void
SetPortValueQueuedEvent::pre_process()
{
	if (m_port == NULL)
		m_port = _engine.object_store()->find_port(m_port_path);

	if (m_port == NULL) {
		m_error = PORT_NOT_FOUND;
	} else if ( !(m_port->type() == DataType::FLOAT) ) {
		m_error = TYPE_MISMATCH;
	}
	
	QueuedEvent::pre_process();
}


void
SetPortValueQueuedEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	assert(_time >= start && _time <= end);

	QueuedEvent::execute(nframes, start, end);

	if (m_error == NO_ERROR) {
		assert(m_port != NULL);
		if (m_voice_num == -1) 
			((TypedPort<Sample>*)m_port)->set_value(m_val, _time - start);
		else
			((TypedPort<Sample>*)m_port)->buffer(m_voice_num)->set(m_val, _time - start); // FIXME: check range
	}
}


void
SetPortValueQueuedEvent::post_process()
{
	if (m_error == NO_ERROR) {
		assert(m_port != NULL);
		
		_responder->respond_ok();
		_engine.client_broadcaster()->send_control_change(m_port_path, m_val);
		
		// Send patch port control change, if this is a bridge port
		/*Port* parent_port = m_port->parent_node()->as_port();
		if (parent_port != NULL) {
			assert(parent_port->type() == DataType::FLOAT);
			_engine.client_broadcaster()->send_control_change(parent_port->path(), m_val);
		}*/

	} else if (m_error == PORT_NOT_FOUND) {
		string msg = "Unable to find port ";
		msg.append(m_port_path).append(" for set_port_value_slow");
		_responder->respond_error(msg);
	
	} else if (m_error == TYPE_MISMATCH) {
		string msg = "Attempt to set ";
		msg.append(m_port_path).append(" to incompatible type");
		_responder->respond_error(msg);
	}
}


} // namespace Ingen

