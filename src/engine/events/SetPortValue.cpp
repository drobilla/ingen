/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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
#include "lv2ext/lv2_event.h"
#include "shared/LV2URIMap.hpp"
#include "shared/LV2Features.hpp"
#include "module/World.hpp"
#include "Responder.hpp"
#include "SetPortValue.hpp"
#include "Engine.hpp"
#include "PortImpl.hpp"
#include "ClientBroadcaster.hpp"
#include "NodeImpl.hpp"
#include "EngineStore.hpp"
#include "AudioBuffer.hpp"
#include "EventBuffer.hpp"
#include "ProcessContext.hpp"
#include "MessageContext.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Events {

using namespace Shared;


/** Omni (all voices) control setting */
SetPortValue::SetPortValue(Engine&              engine,
                                     SharedPtr<Responder> responder,
                                     bool                 queued,
                                     SampleCount          timestamp,
                                     const Raul::Path&    port_path,
                                     const Raul::Atom&    value)
	: QueuedEvent(engine, responder, timestamp)
	, _queued(queued)
	, _omni(true)
	, _voice_num(0)
	, _port_path(port_path)
    , _value(value)
	, _port(NULL)
	, _error(NO_ERROR)
{
}


/** Voice-specific control setting */
SetPortValue::SetPortValue(Engine&              engine,
                                     SharedPtr<Responder> responder,
                                     bool                 queued,
                                     SampleCount          timestamp,
                                     uint32_t             voice_num,
                                     const Raul::Path&    port_path,
                                     const Raul::Atom&    value)
	: QueuedEvent(engine, responder, timestamp)
	, _queued(queued)
	, _omni(false)
	, _voice_num(voice_num)
	, _port_path(port_path)
    , _value(value)
	, _port(NULL)
	, _error(NO_ERROR)
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
		apply(0, 0);
		_engine.message_context()->run(_port->parent_node());
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

	apply(context.start(), context.nframes());
}


void
SetPortValue::apply(uint32_t start, uint32_t nframes)
{
	if (_error == NO_ERROR && !_port)
		_port = _engine.engine_store()->find_port(_port_path);

	if (!_port) {
		if (_error == NO_ERROR)
			_error = PORT_NOT_FOUND;
	/*} else if (_port->buffer(0)->capacity() < _data_size) {
		_error = NO_SPACE;*/
	} else {
		Buffer* const buf = _port->buffer(0);
		AudioBuffer* const abuf = dynamic_cast<AudioBuffer*>(buf);
		if (abuf) {
			if (_value.type() != Atom::FLOAT) {
				_error = TYPE_MISMATCH;
				return;
			}

			if (_omni) {
				for (uint32_t i=0; i < _port->poly(); ++i)
					((AudioBuffer*)_port->buffer(i))->set_value(
							_value.get_float(), start, _time);
			} else {
				if (_voice_num < _port->poly())
					((AudioBuffer*)_port->buffer(_voice_num))->set_value(
							_value.get_float(), start, _time);
				else
					_error = ILLEGAL_VOICE;
			}
			return;
		}

		EventBuffer* const ebuf = dynamic_cast<EventBuffer*>(buf);

		const LV2Features::Feature* f = _engine.world()->lv2_features->feature(LV2_URI_MAP_URI);
		LV2URIMap* map = (LV2URIMap*)f->controller;

		// FIXME: eliminate lookups
		// FIXME: need a proper prefix system
		if (ebuf && _value.type() == Atom::BLOB) {
			const uint32_t frames = std::max(
					(uint32_t)(_time - start),
					ebuf->latest_frames());

			// Size 0 event, pass it along to the plugin as a typed but empty event
			if (_value.data_size() == 0) {
				cout << "BANG!" << endl;
				const uint32_t type_id = map->uri_to_id(NULL, _value.get_blob_type());
				ebuf->append(frames, 0, type_id, 0, NULL);
				_port->raise_set_by_user_flag();
				return;

			} else if (!strcmp(_value.get_blob_type(), "lv2midi:MidiEvent")) {
				const uint32_t type_id = map->uri_to_id(NULL,
						"http://lv2plug.in/ns/ext/midi#MidiEvent");

				ebuf->prepare_write(start, nframes);
				// FIXME: use OSC midi type? avoid MIDI over OSC entirely?
				ebuf->append(frames, 0, type_id, _value.data_size(),
						(const uint8_t*)_value.get_blob());
				_port->raise_set_by_user_flag();
				return;
			}
		}

		if (_value.type() == Atom::BLOB)
			cerr << "WARNING: Unknown value blob type " << _value.get_blob_type() << endl;
		else
			cerr << "WARNING: Unknown value type " << (int)_value.type() << endl;
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
		_responder->respond_ok();
		_engine.broadcaster()->send_port_value(_port_path, _value);
		break;
	case TYPE_MISMATCH:
		_responder->respond_error("type mismatch");
		break;
	case ILLEGAL_VOICE:
		ss << "Illegal voice number " << _voice_num;
		_responder->respond_error(ss.str());
		break;
	case PORT_NOT_FOUND:
		msg = "Unable to find port ";
		msg.append(_port_path.str()).append(" for set_port_value");
		_responder->respond_error(msg);
		break;
	case NO_SPACE:
		ss << "Attempt to write " << _value.data_size() << " bytes to "
			<< _port_path.str() << ", with capacity "
			<< _port->buffer_size() << endl;
		_responder->respond_error(ss.str());
		break;
	}
}


} // namespace Ingen
} // namespace Events

