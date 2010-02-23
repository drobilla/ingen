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

#ifndef INGEN_EVENTS_SENDBINDING_HPP
#define INGEN_EVENTS_SENDBINDING_HPP

#include "raul/Atom.hpp"
#include "engine/Event.hpp"
#include "engine/ControlBindings.hpp"
#include "engine/types.hpp"

namespace Ingen {

class PortImpl;

namespace Events {


/** A special event used internally to send control bindings from the audio thread.
 *
 * See SendPortValue documentation for details.
 *
 * \ingroup engine
 */
class SendBinding : public Event
{
public:
	inline SendBinding(
			Engine&                    engine,
			SampleCount                timestamp,
			PortImpl*                  port,
			ControlBindings::Type      type,
			int16_t                    num)
		: Event(engine, SharedPtr<Request>(), timestamp)
		, _port(port)
		, _type(type)
		, _num(num)
	{
		assert(_port);
		switch (type) {
		case ControlBindings::MIDI_CC:
			assert(num >= 0 && num < 128);
			break;
		case ControlBindings::MIDI_RPN:
			assert(num >= 0 && num < 16384);
			break;
		case ControlBindings::MIDI_NRPN:
			assert(num >= 0 && num < 16384);
		default:
			break;
		}
	}

	inline SendBinding& operator=(const SendBinding& ev) {
		_port = ev._port;
		_type = ev._type;
		_num  = ev._num;
		return *this;
	}

	void post_process();

private:
	PortImpl*             _port;
	ControlBindings::Type _type;
	int16_t               _num;
};


} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_SENDBINDING_HPP
