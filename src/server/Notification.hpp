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

#ifndef INGEN_ENGINE_NOTIFICATION_HPP
#define INGEN_ENGINE_NOTIFICATION_HPP

#include "raul/Atom.hpp"

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
