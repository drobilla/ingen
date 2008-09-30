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
#include "SendPortValueEvent.hpp"
#include "Engine.hpp"
#include "PortImpl.hpp"
#include "ClientBroadcaster.hpp"

using namespace std;

namespace Ingen {


void
SendPortValueEvent::post_process()
{
	// FIXME...
	
	if (_omni) {
		_engine.broadcaster()->send_port_value(_port->path(), _value);
	} else {
		_engine.broadcaster()->send_port_value(_port->path(), _value);
	}
}


} // namespace Ingen

