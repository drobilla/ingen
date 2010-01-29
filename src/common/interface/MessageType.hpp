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

#ifndef MESSAGE_TYPE_HPP
#define MESSAGE_TYPE_HPP

#include <cassert>
#include <iostream>
#include <string>

namespace Ingen {
namespace Shared {


/** A type of control message that could be bound to a control port.
 *
 * \ingroup interface
 */
class MessageType
{
public:
	enum Type {
		MIDI_PITCH,
		MIDI_CC,
		MIDI_RPN,
		MIDI_NRPN
	};

	MessageType(Type type, int16_t num)
		: _type(type)
	{
		switch (type) {
		case MIDI_PITCH:
			break;
		case MIDI_CC:
			assert(num >= 0 && num < 128);
			_id.midi_cc = num;
			break;
		case MIDI_RPN:
			assert(num >= 0 && num < 16384);
			_id.midi_pn = num;
			break;
		case MIDI_NRPN:
			assert(num >= 0 && num < 16384);
			_id.midi_pn = num;
			break;
		}
	}

	inline Type   type()          const { return _type; }
	inline int8_t midi_cc_num()   const { assert(_type == MIDI_CC);   return _id.midi_cc; }
	inline int8_t midi_rpn_num()  const { assert(_type == MIDI_RPN);  return _id.midi_pn; }
	inline int8_t midi_nrpn_num() const { assert(_type == MIDI_NRPN); return _id.midi_pn; }

	inline int num() const {
		switch (_type) {
		case MIDI_CC:
			return _id.midi_cc;
		case MIDI_RPN:
		case MIDI_NRPN:
			return _id.midi_pn;
		default:
			return -1;
		}
	}

	inline const char* type_uri() const {
		switch (_type) {
		case MIDI_PITCH:
			return "midi:PitchBend";
		case MIDI_CC:
			return "midi:Control";
		case MIDI_RPN:
			return "midi:RPN";
		case MIDI_NRPN:
			return "midi:NRPN";
		}
	}

private:
	union {
		int8_t  midi_cc; ///< Controller number [0..2^7)
		int16_t midi_pn; ///< RPN or NRPN number [0..2^14)
	} _id;

	Type _type;
};


} // namespace Shared
} // namespace Ingen


static inline std::ostream& operator<<(std::ostream& os, const Ingen::Shared::MessageType& type)
{
	using namespace Ingen::Shared;
	switch (type.type()) {
	case MessageType::MIDI_PITCH: return os << "MIDI Pitch Bender";
	case MessageType::MIDI_CC:    return os << "MIDI CC " << type.num();
	case MessageType::MIDI_RPN:   return os << "MIDI RPN " << type.num();
	case MessageType::MIDI_NRPN:  return os << "MIDI NRPN " << type.num();
	}
	return os;
}

#endif // MESSAGE_TYPE_HPP
