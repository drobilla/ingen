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
#include "ingen/Log.hpp"
#include "ingen/URIMap.hpp"

#include "Broadcaster.hpp"
#include "BufferFactory.hpp"
#include "Context.hpp"
#include "Engine.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {

struct Notification
{
	inline Notification(PortImpl* p = 0,
	                    FrameTime f = 0,
	                    LV2_URID  k = 0,
	                    uint32_t  s = 0,
	                    LV2_URID  t = 0)
		: port(p), time(f), key(k), size(s), type(t)
	{}

	PortImpl* port;
	FrameTime time;
	LV2_URID  key;
	uint32_t  size;
	LV2_URID  type;
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

bool
Context::must_notify(const PortImpl* port) const
{
	return (port->is_monitored() || _engine.broadcaster()->must_broadcast());
}

bool
Context::notify(LV2_URID    key,
                FrameTime   time,
                PortImpl*   port,
                uint32_t    size,
                LV2_URID    type,
                const void* body)
{
	const Notification n(port, time, key, size, type);
	if (_event_sink.write_space() < sizeof(n) + size) {
		return false;
	}
	if (_event_sink.write(sizeof(n), &n) != sizeof(n)) {
		_engine.log().error("Error writing header to notification ring\n");
	} else if (_event_sink.write(size, body) != size) {
		_engine.log().error("Error writing body to notification ring\n");
	} else {
		return true;
	}
	return false;
}

void
Context::emit_notifications(FrameTime end)
{
	const URIs&    uris       = _engine.buffer_factory()->uris();
	const uint32_t read_space = _event_sink.read_space();
	Notification   note;
	for (uint32_t i = 0; i < read_space; i += sizeof(note)) {
		if (_event_sink.peek(sizeof(note), &note) != sizeof(note) ||
		    note.time >= end) {
			return;
		}
		if (_event_sink.read(sizeof(note), &note) == sizeof(note)) {
			Atom value = _engine.world()->forge().alloc(
				note.size, note.type, NULL);
			if (_event_sink.read(note.size, value.get_body()) == note.size) {
				i += note.size;
				const char* key = _engine.world()->uri_map().unmap_uri(note.key);
				if (key) {
					_engine.broadcaster()->set_property(
						note.port->uri(), Raul::URI(key), value);
					if (note.port->is_input() && note.key == uris.ingen_value) {
						// FIXME: not thread safe
						note.port->set_property(uris.ingen_value, value);
					}
				} else {
					_engine.log().error("Error unmapping notification key URI\n");
				}
			} else {
				_engine.log().error("Error reading body from notification ring\n");
			}
		} else {
			_engine.log().error("Error reading header from notification ring\n");
		}
	}
}

} // namespace Server
} // namespace Ingen
