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

#include "ingen/Forge.hpp"
#include "ingen/URIMap.hpp"
#include "raul/log.hpp"

#include "Context.hpp"
#include "Engine.hpp"
#include "Broadcaster.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {

struct Notification
{
	inline Notification(PortImpl*          p = 0,
	                    FrameTime          f = 0,
	                    LV2_URID           k = 0,
	                    uint32_t           s = 0,
	                    Raul::Atom::TypeID t = 0)
		: port(p), time(f), key(k), size(s), type(t)
	{}

	PortImpl*          port;
	FrameTime          time;
	LV2_URID           key;
	uint32_t           size;
	Raul::Atom::TypeID type;
};

Context::Context(Engine& engine, ID id)
	: _engine(engine)
	, _id(id)
	, _event_sink(engine.event_queue_size() * sizeof(Notification))
	, _start(0)
	, _end(0)
	, _nframes(0)
	, _realtime(true)
{}

void
Context::notify(LV2_URID           key,
                FrameTime          time,
                PortImpl*          port,
                uint32_t           size,
                Raul::Atom::TypeID type,
                const void*        body)
{
	const Notification n(port, time, key, size, type);
	if (_event_sink.write_space() < sizeof(n) + size) {
		Raul::warn("Notification ring overflow\n");
	} else {
		if (_event_sink.write(sizeof(n), &n) != sizeof(n)) {
			Raul::error("Error writing header to notification ring\n");
		} else if (_event_sink.write(size, body) != size) {
			Raul::error("Error writing body to notification ring\n");
		}
	}
}

void
Context::emit_notifications(FrameTime end)
{
	const uint32_t read_space = _event_sink.read_space();
	Notification   note;
	for (uint32_t i = 0; i < read_space; i += sizeof(note)) {
		if (_event_sink.peek(sizeof(note), &note) != sizeof(note) ||
		    note.time >= end) {
			return;
		}
		if (_event_sink.read(sizeof(note), &note) == sizeof(note)) {
			Raul::Atom value = _engine.world()->forge().alloc(
				note.size, note.type, NULL);
			if (_event_sink.read(note.size, value.get_body()) == note.size) {
				i += note.size;
				const char* key = _engine.world()->uri_map().unmap_uri(note.key);
				if (key) {
					_engine.broadcaster()->set_property(
						note.port->uri(), key, value);
				} else {
					Raul::error("Error unmapping notification key URI\n");
				}
			} else {
				Raul::error("Error reading body from notification ring\n");
			}
		} else {
			Raul::error("Error reading header from notification ring\n");
		}
	}
}

} // namespace Server
} // namespace Ingen
