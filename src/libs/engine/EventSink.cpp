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

#include <iostream>
#include "events/SendPortValueEvent.hpp"
#include "EventSink.hpp"
#include "Port.hpp"

using namespace std;

namespace Ingen {
	
	
void
EventSink::control_change(Port* port, FrameTime time, float val)
{
	//cerr << "CONTROL CHANGE: " << port->path() << " == " << val << endl;
	SendPortValueEvent ev(_engine, time, port, false, 0, val);
	_events.write(sizeof(ev), (uchar*)&ev);
}
	

bool
EventSink::read_control_change(SendPortValueEvent& ev)
{
	return _events.full_read(sizeof(SendPortValueEvent), (uchar*)&ev);
}


} // namespace Ingen
