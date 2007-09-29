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
#include "Responder.hpp"
#include "SendPortValueEvent.hpp"
#include "Engine.hpp"
#include "Port.hpp"
#include "ClientBroadcaster.hpp"
#include "Node.hpp"
#include "ObjectStore.hpp"
#include "AudioBuffer.hpp"
#include "MidiBuffer.hpp"

using namespace std;

namespace Ingen {


SendPortValueEvent(Engine&     engine,
                   SampleCount timestamp,
                   Port*       port,
                   bool        omni,
                   uint32_t    voice_num,
				   float       value)
	: _port(port)
	, _omni(omni)
	, _voice_num(voice_num)
	, _value(value)
{
}


void
SendPortValueEvent::post_process()
{
	if (_omni) {
		_engine.broadcaster()->send_control_change(_port->path(), _value);
	} else {
		cerr << "NON-OMNI CONTROL CHANGE WHAT?" << endl;
		_engine.broadcaster()->send_control_change(_port->path(), _value);
	}
}


} // namespace Ingen

