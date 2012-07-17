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

#include "Context.hpp"

namespace Ingen {
namespace Server {

void
Context::notify(Notification::Type          type,
                FrameTime                   time,
                PortImpl*                   port,
                const Raul::Atom&           value,
                const ControlBindings::Type btype)
{
	const Notification n = Notification::make(type, time, port, value, btype);
	_event_sink.write(sizeof(n), &n);
}

void
Context::emit_notifications()
{
	const uint32_t read_space = _event_sink.read_space();
	Notification   note;
	for (uint32_t i = 0; i < read_space; i += sizeof(note)) {
		if (_event_sink.read(sizeof(note), &note) == sizeof(note)) {
			Notification::post_process(note, _engine);
		}
	}
}

} // namespace Server
} // namespace Ingen
