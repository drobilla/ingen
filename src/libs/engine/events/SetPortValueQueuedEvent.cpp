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
#include "SetPortValueQueuedEvent.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "Port.hpp"
#include "ClientBroadcaster.hpp"
#include "Plugin.hpp"
#include "Node.hpp"
#include "ObjectStore.hpp"
#include "AudioBuffer.hpp"
#include "MidiBuffer.hpp"
#include "ProcessContext.hpp"

namespace Ingen {


/** Omni (all voices) control setting */
SetPortValueQueuedEvent::SetPortValueQueuedEvent(Engine&              engine,
                                                 SharedPtr<Responder> responder,
                                                 SampleCount          timestamp,
                                                 const string&        port_path,
                                                 uint32_t             data_size,
                                                 const void*          data)
: QueuedEvent(engine, responder, timestamp),
  _omni(true),
  _voice_num(0),
  _port_path(port_path),
  _data_size(data_size),
  _data(malloc(data_size)),
  _port(NULL),
  _error(NO_ERROR)
{
	memcpy(_data, data, data_size);
}


/** Voice-specific control setting */
SetPortValueQueuedEvent::SetPortValueQueuedEvent(Engine&              engine,
                                                 SharedPtr<Responder> responder,
                                                 SampleCount          timestamp,
                                                 uint32_t             voice_num,
                                                 const string&        port_path,
                                                 uint32_t             data_size,
                                                 const void*          data)
: QueuedEvent(engine, responder, timestamp),
  _omni(false),
  _voice_num(voice_num),
  _port_path(port_path),
  _data_size(data_size),
  _data(malloc(data_size)),
  _port(NULL),
  _error(NO_ERROR)
{
	memcpy(_data, data, data_size);
}


void
SetPortValueQueuedEvent::pre_process()
{
	if (_port == NULL)
		_port = _engine.object_store()->find_port(_port_path);

	if (_port == NULL) {
		_error = PORT_NOT_FOUND;
/*	} else if (_port->buffer_size() < _data_size) {
		_error = NO_SPACE;*/
	}
	
	QueuedEvent::pre_process();
}


void
SetPortValueQueuedEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);
	assert(_time >= context.start() && _time <= context.end());

	if (_error == NO_ERROR) {
		assert(_port);
		
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
		
		MidiBuffer* const mbuf = dynamic_cast<MidiBuffer*>(buf);
		if (mbuf) {
			const double stamp = std::max((double)(_time - context.start()), mbuf->latest_stamp());
			mbuf->append(stamp, _data_size, (const unsigned char*)_data);
		}
	}
}


void
SetPortValueQueuedEvent::post_process()
{
	if (_error == NO_ERROR) {
		assert(_port != NULL);
		
		_responder->respond_ok();
		_engine.broadcaster()->send_control_change(_port_path, *(float*)_data);
		
		// Send patch port control change, if this is a bridge port
		/*Port* parent_port = _port->parent_node()->as_port();
		if (parent_port != NULL) {
			assert(parent_port->type() == DataType::FLOAT);
			_engine.broadcaster()->send_control_change(parent_port->path(), _val);
		}*/

	} else if (_error == PORT_NOT_FOUND) {
		string msg = "Unable to find port ";
		msg.append(_port_path).append(" for set_port_value_slow");
		_responder->respond_error(msg);
	
	} else if (_error == NO_SPACE) {
		std::ostringstream msg("Attempt to write ");
		msg << _data_size << " bytes to " << _port_path << ", with capacity "
			<< _port->buffer_size() << endl;
		_responder->respond_error(msg.str());
	}
}


} // namespace Ingen

