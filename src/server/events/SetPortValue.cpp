/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/LV2Features.hpp"
#include "ingen/Store.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"

#include "BlockImpl.hpp"
#include "Broadcaster.hpp"
#include "Buffer.hpp"
#include "ControlBindings.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
#include "SetPortValue.hpp"

namespace Ingen {
namespace Server {
namespace Events {

/** Internal */
SetPortValue::SetPortValue(Engine&         engine,
                           SPtr<Interface> client,
                           int32_t         id,
                           SampleCount     timestamp,
                           PortImpl*       port,
                           const Atom&     value)
	: Event(engine, client, id, timestamp)
	, _port(port)
	, _value(value)
{
}

SetPortValue::~SetPortValue()
{
}

bool
SetPortValue::pre_process()
{
	if (_port->is_output()) {
		return Event::pre_process_done(Status::DIRECTION_MISMATCH, _port->path());
	}

	// Set value metadata (does not affect buffers)
	_port->set_value(_value);
	_port->set_property(_engine.world()->uris().ingen_value, _value);

	_binding = _engine.control_bindings()->port_binding(_port);

	return Event::pre_process_done(Status::SUCCESS);
}

void
SetPortValue::execute(ProcessContext& context)
{
	assert(_time >= context.start() && _time <= context.end());

	if (_port->parent_block()->context() == Context::ID::MESSAGE)
		return;

	apply(context);
	_engine.control_bindings()->port_value_changed(context, _port, _binding, _value);
}

void
SetPortValue::apply(Context& context)
{
	if (_status != Status::SUCCESS) {
		return;
	}

	Ingen::URIs&  uris = _engine.world()->uris();
	Buffer* const buf  = _port->buffer(0).get();

	if (buf->type() == uris.atom_Sound || buf->type() == uris.atom_Float) {
		if (_value.type() == uris.forge.Float) {
			_port->set_control_value(context, _time, _value.get<float>());
		} else {
			_status = Status::TYPE_MISMATCH;
		}
	} else if (buf->type() == uris.atom_Sequence) {
		buf->prepare_write(context);  // FIXME: incorrect
		if (buf->append_event(_time - context.start(),
		                      _value.size(),
		                      _value.type(),
		                      (const uint8_t*)_value.get_body())) {
			_port->raise_set_by_user_flag();
		} else {
			_status = Status::NO_SPACE;
		}
	} else if (buf->type() == uris.atom_URID) {
		((LV2_Atom_URID*)buf->atom())->body = _value.get<int32_t>();
	} else {
		_status = Status::BAD_VALUE_TYPE;
	}
}

void
SetPortValue::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS) {
		_engine.broadcaster()->set_property(
			_port->uri(),
			_engine.world()->uris().ingen_value,
			_value);
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen

