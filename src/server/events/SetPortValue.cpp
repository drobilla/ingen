/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include "ingen/shared/LV2Atom.hpp"
#include "ingen/shared/LV2Features.hpp"
#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/shared/URIs.hpp"
#include "ingen/shared/World.hpp"
#include "raul/log.hpp"

#include "AudioBuffer.hpp"
#include "ClientBroadcaster.hpp"
#include "ControlBindings.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "MessageContext.hpp"
#include "NodeImpl.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
#include "SetPortValue.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {
namespace Events {

SetPortValue::SetPortValue(Engine&           engine,
                           Interface*        client,
                           int32_t           id,
                           bool              queued,
                           SampleCount       timestamp,
                           const Raul::Path& port_path,
                           const Raul::Atom& value)
	: Event(engine, client, id, timestamp)
	, _queued(queued)
	, _port_path(port_path)
	, _value(value)
	, _port(NULL)
{
}

/** Internal */
SetPortValue::SetPortValue(Engine&           engine,
                           Interface*        client,
                           int32_t           id,
                           SampleCount       timestamp,
                           PortImpl*         port,
                           const Raul::Atom& value)
	: Event(engine, client, id, timestamp)
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
			_status = PORT_NOT_FOUND;
	}

	// Port is a message context port, set its value and
	// call the plugin's message run function once
	if (_port && _port->context() == Context::MESSAGE) {
		apply(*_engine.message_context());
		_port->parent_node()->set_port_valid(_port->index());
		_engine.message_context()->run(_port->parent_node(),
				_engine.driver()->frame_time() + _engine.driver()->block_length());
	}

	if (_port) {
		_port->set_value(_value);
		_port->set_property(_engine.world()->uris()->ingen_value, _value);
	}

	_binding = _engine.control_bindings()->port_binding(_port);

	Event::pre_process();
}

void
SetPortValue::execute(ProcessContext& context)
{
	Event::execute(context);
	assert(_time >= context.start() && _time <= context.end());

	if (_port && _port->context() == Context::MESSAGE)
		return;

	apply(context);
	_engine.control_bindings()->port_value_changed(context, _port, _binding, _value);
}

void
SetPortValue::apply(Context& context)
{
	uint32_t start = context.start();
	if (_status == SUCCESS && !_port)
		_port = _engine.engine_store()->find_port(_port_path);

	Ingen::Shared::URIs& uris = *_engine.world()->uris().get();

	if (!_port) {
		if (_status == SUCCESS)
			_status = PORT_NOT_FOUND;
	/*} else if (_port->buffer(0)->capacity() < _data_size) {
		_status = NO_SPACE;*/
	} else {
		Buffer* const      buf  = _port->buffer(0).get();
		AudioBuffer* const abuf = dynamic_cast<AudioBuffer*>(buf);
		if (abuf) {
			if (_value.type() != uris.forge.Float) {
				_status = TYPE_MISMATCH;
				return;
			}

			for (uint32_t v = 0; v < _port->poly(); ++v) {
				((AudioBuffer*)_port->buffer(v).get())->set_value(
						_value.get_float(), start, _time);
			}
			return;
		}

		warn << "Unknown value type " << (int)_value.type() << endl;
	}
}

void
SetPortValue::post_process()
{
	respond(_status);
	if (!_status) {
		_engine.broadcaster()->set_property(
			_port_path,
			_engine.world()->uris()->ingen_value,
			_value);
	}
}

} // namespace Server
} // namespace Ingen
} // namespace Events

