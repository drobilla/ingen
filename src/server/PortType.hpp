/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_PORTTYPE_HPP
#define INGEN_ENGINE_PORTTYPE_HPP

#include "ingen/URI.hpp"

#include "lv2/atom/atom.h"
#include "lv2/core/lv2.h"

namespace ingen {

/// The type of a port
enum class PortType {
	UNKNOWN,
	AUDIO,
	CONTROL,
	CV,
	ATOM,
};

/// Return the URI for `port_type`
inline URI
port_type_uri(const PortType port_type)
{
	switch (port_type) {
	case PortType::UNKNOWN:
		break;
	case PortType::AUDIO:
		return URI{LV2_CORE__AudioPort};
	case PortType::CONTROL:
		return URI{LV2_CORE__ControlPort};
	case PortType::CV:
		return URI{LV2_CORE__CVPort};
	case PortType::ATOM:
		return URI{LV2_ATOM__AtomPort};
	}

	return URI{"http://www.w3.org/2002/07/owl#Nothing"};
}

/// Return the type with the given `uri`, or #PortType::UNKNOWN
inline PortType
port_type_from_uri(const URI& uri)
{
	static const URI lv2_AudioPort   = URI{LV2_CORE__AudioPort};
	static const URI lv2_ControlPort = URI{LV2_CORE__ControlPort};
	static const URI lv2_CVPort      = URI{LV2_CORE__CVPort};
	static const URI atom_AtomPort   = URI{LV2_ATOM__AtomPort};

	return (uri == lv2_AudioPort)     ? PortType::AUDIO
	       : (uri == lv2_ControlPort) ? PortType::CONTROL
	       : (uri == lv2_CVPort)      ? PortType::CV
	       : (uri == atom_AtomPort)   ? PortType::ATOM
	                                  : PortType::UNKNOWN;
}

} // namespace ingen

#endif // INGEN_ENGINE_PORTTYPE_HPP
