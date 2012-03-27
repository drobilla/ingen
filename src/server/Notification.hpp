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


#ifndef INGEN_ENGINE_NOTIFICATION_HPP
#define INGEN_ENGINE_NOTIFICATION_HPP

#include "ControlBindings.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {

class Engine;
class PortImpl;

struct Notification
{
	enum Type {
		NIL,
		PORT_VALUE,
		PORT_ACTIVITY,
		PORT_BINDING
	};

	static inline Notification make(
		Type                        type  = NIL,
		FrameTime                   time  = 0,
		PortImpl*                   port  = 0,
		const Raul::Atom&           value = Raul::Atom(),
		const ControlBindings::Type btype = ControlBindings::NULL_CONTROL)
	{
		const Notification note = { port, type, btype, value };
		return note;
	}

	static void post_process(Notification& note,
	                         Engine&       engine);

	PortImpl*             port;
	Type                  type;
	ControlBindings::Type binding_type;
	Raul::Atom            value;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_NOTIFICATION_HPP
