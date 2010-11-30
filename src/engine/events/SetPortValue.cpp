/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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
#include "raul/log.hpp"
#include "lv2/lv2plug.in/ns/ext/event/event.h"
#include "shared/LV2URIMap.hpp"
#include "shared/LV2Features.hpp"
#include "shared/LV2Atom.hpp"
#include "module/World.hpp"
#include "AudioBuffer.hpp"
#include "ClientBroadcaster.hpp"
#include "ControlBindings.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "EventBuffer.hpp"
#include "MessageContext.hpp"
#include "NodeImpl.hpp"
#include "ObjectBuffer.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
#include "ProcessContext.hpp"
#include "Request.hpp"
#include "SetPortValue.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Events {

using namespace Shared;


SetPortValue::SetPortValue(Engine&            engine,
                           SharedPtr<Request> request,
                           bool               queued,
                           SampleCount        timestamp,
                           const Raul::Path&  port_path,
                           const Raul::Atom&  value)
	: QueuedEvent(engine, request, timestamp)
	, _queued(queued)
	, _port_path(port_path)
	, _value(value)
	, _port(NULL)
{
}


/** Internal */
SetPortValue::SetPortValue(Engine&            engine,
                           SharedPtr<Request> request,
                           SampleCount        timestamp,
                           PortImpl*          port,
                           const Raul::Atom&  value)
	: QueuedEvent(engine, request, timestamp)
	, _queued(false)
	, _port_path(port->path())
	, _value(value)
	, _port(port)
{
}

SetPortValue::~SetPortValue()
{
}


void
SetPortValue::pre_process()
{
	if (_queued) {
		if (_port == NULL)
			_port = _engine.engine_store()->find_port(_port_path);
		if (_port == NULL)
			_error = PORT_NOT_FOUND;
	}

	// Port is a message context port, set its value and
	// call the plugin's message run function once
	if (_port && _port->context() == Context::MESSAGE) {
		apply(*_engine.message_context());
		_port->parent_node()->set_port_valid(_port->index());
		_engine.message_context()->run(_port,
				_engine.driver()->frame_time() + _engine.driver()->block_length());
	}

	if (_port) {
		_port->set_value(_value);
		_port->set_property(_engine.world()->uris()->ingen_value, _value);
	}

	QueuedEvent::pre_process();
}


void
SetPortValue::execute(ProcessContext& context)
{
	Event::execute(context);
	assert(_time >= context.start() && _time <= context.end());

	if (_port && _port->context() == Context::MESSAGE)
		return;

	apply(context);
	_engine.control_bindings()->port_value_changed(context, _port);
}


void
SetPortValue::apply(Context& context)
{
	uint32_t start = context.start();
	if (_error == NO_ERROR && !_port)
		_port = _engine.engine_store()->find_port(_port_path);

	if (!_port) {
		if (_error == NO_ERROR)
			_error = PORT_NOT_FOUND;
	/*} else if (_port->buffer(0)->capacity() < _data_size) {
		_error = NO_SPACE;*/
	} else {
		Buffer* const      buf  = _port->buffer(0).get();
		AudioBuffer* const abuf = dynamic_cast<AudioBuffer*>(buf);
		if (abuf) {
			if (_value.type() != Atom::FLOAT) {
				_error = TYPE_MISMATCH;
				return;
			}

			for (uint32_t v = 0; v < _port->poly(); ++v) {
				((AudioBuffer*)_port->buffer(v).get())->set_value(
						_value.get_float(), start, _time);
			}
			return;
		}

		LV2URIMap& uris = *_engine.world()->uris().get();

		EventBuffer* const ebuf = dynamic_cast<EventBuffer*>(buf);
		if (ebuf) {
			const uint32_t frames = std::max(uint32_t(_time - start), ebuf->latest_frames());

			// Size 0 event, pass it along to the plugin as a typed but empty event
			if (_value.data_size() == 0) {
				const uint32_t type_id = uris.uri_to_id(NULL, _value.get_blob_type());
				ebuf->append(frames, 0, type_id, 0, NULL);
				_port->raise_set_by_user_flag();
				return;

			} else if (!strcmp(_value.get_blob_type(), "http://lv2plug.in/ns/ext/midi#MidiEvent")) {
				ebuf->prepare_write(context);
				ebuf->append(frames, 0, uris.midi_MidiEvent.id, _value.data_size(),
						(const uint8_t*)_value.get_blob());
				_port->raise_set_by_user_flag();
				return;
			}
		}

		ObjectBuffer* const obuf = dynamic_cast<ObjectBuffer*>(buf);
		if (obuf) {
			obuf->atom()->size = obuf->size() - sizeof(LV2_Atom);
			if (LV2Atom::from_atom(uris, _value, obuf->atom())) {
				debug << "Converted atom " << _value << " :: " << obuf->atom()->type
					<< " * " << obuf->atom()->size << " @ " << obuf->atom() << endl;
				return;
			} else {
				warn << "Failed to convert atom to LV2 object" << endl;
			}
		}

		warn << "Unknown value type " << (int)_value.type() << endl;
	}
}


void
SetPortValue::post_process()
{
	string msg;
	std::ostringstream ss;
	switch (_error) {
	case NO_ERROR:
		assert(_port != NULL);
		_request->respond_ok();
		_engine.broadcaster()->set_property(_port_path,
				_engine.world()->uris()->ingen_value, _value);
		break;
	case TYPE_MISMATCH:
		ss << "Illegal value type " << _value.type()
			<< " for port " << _port_path << endl;
		_request->respond_error(ss.str());
		break;
	case PORT_NOT_FOUND:
		msg = "Unable to find port ";
		msg.append(_port_path.str()).append(" to set value");
		_request->respond_error(msg);
		break;
	case NO_SPACE:
		ss << "Attempt to write " << _value.data_size() << " bytes to "
			<< _port_path.str() << ", with capacity "
			<< _port->buffer_size() << endl;
		_request->respond_error(ss.str());
		break;
	}
}


} // namespace Ingen
} // namespace Events

