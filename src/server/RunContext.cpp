/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "RunContext.hpp"

#include "Broadcaster.hpp"
#include "BufferFactory.hpp"
#include "Engine.hpp"
#include "PortImpl.hpp"
#include "Task.hpp"

#include "ingen/Atom.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Log.hpp"
#include "ingen/URI.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "lv2/urid/urid.h"
#include "raul/RingBuffer.hpp"

#include <cerrno>
#include <cstring>
#include <pthread.h>
#include <sched.h>

namespace ingen {
namespace server {

struct Notification {
	explicit inline Notification(PortImpl* p = nullptr,
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

RunContext::RunContext(Engine&           engine,
                       raul::RingBuffer* event_sink,
                       unsigned          id,
                       bool              threaded)
	: _engine(engine)
	, _event_sink(event_sink)
	, _thread(threaded ? new std::thread(&RunContext::run, this) : nullptr)
	, _id(id)
{}

RunContext::RunContext(const RunContext& copy)
	: _engine(copy._engine)
	, _event_sink(copy._event_sink)
	, _id(copy._id)
	, _start(copy._start)
	, _end(copy._end)
	, _offset(copy._offset)
	, _nframes(copy._nframes)
	, _rate(copy._rate)
	, _realtime(copy._realtime)
{}

bool
RunContext::must_notify(const PortImpl* port) const
{
	return (port->is_monitored() || _engine.broadcaster()->must_broadcast());
}

bool
RunContext::notify(LV2_URID    key,
                   FrameTime   time,
                   PortImpl*   port,
                   uint32_t    size,
                   LV2_URID    type,
                   const void* body)
{
	const Notification n(port, time, key, size, type);
	if (_event_sink->write_space() < sizeof(n) + size) {
		return false;
	}
	if (_event_sink->write(sizeof(n), &n) != sizeof(n)) {
		_engine.log().rt_error("Error writing header to notification ring\n");
	} else if (_event_sink->write(size, body) != size) {
		_engine.log().rt_error("Error writing body to notification ring\n");
	} else {
		return true;
	}
	return false;
}

void
RunContext::emit_notifications(FrameTime end)
{
	const URIs&    uris       = _engine.buffer_factory()->uris();
	const uint32_t read_space = _event_sink->read_space();
	Notification   note;
	for (uint32_t i = 0; i < read_space; i += sizeof(note)) {
		if (_event_sink->peek(sizeof(note), &note) != sizeof(note) ||
		    note.time >= end) {
			return;
		}
		if (_event_sink->read(sizeof(note), &note) == sizeof(note)) {
			Atom value = Forge::alloc(note.size, note.type, nullptr);
			if (_event_sink->read(note.size, value.get_body()) == note.size) {
				i += note.size;
				const char* key = _engine.world().uri_map().unmap_uri(note.key);
				if (key) {
					_engine.broadcaster()->set_property(
						note.port->uri(), URI(key), value);
					if (note.port->is_input() &&
					    (note.key == uris.ingen_value ||
					     note.key == uris.midi_binding)) {
						// FIXME: not thread safe
						note.port->set_property(URI(key), value);
					}
				} else {
					_engine.log().rt_error("Error unmapping notification key URI\n");
				}
			} else {
				_engine.log().rt_error("Error reading body from notification ring\n");
			}
		} else {
			_engine.log().rt_error("Error reading header from notification ring\n");
		}
	}
}

void
RunContext::claim_task(Task* task)
{
	if ((_task = task)) {
		_engine.signal_tasks_available();
	}
}

Task*
RunContext::steal_task() const
{
	return _engine.steal_task(_id + 1);
}

void
RunContext::set_priority(int priority)
{
	if (_thread) {
		pthread_t   pthread = _thread->native_handle();
		const int   policy  = (priority > 0) ? SCHED_FIFO : SCHED_OTHER;
		sched_param sp{};
		sp.sched_priority = (priority > 0) ? priority : 0;
		if (pthread_setschedparam(pthread, policy, &sp)) {
			_engine.log().error(
				"Failed to set real-time priority of run thread (%s)\n",
				strerror(errno));
		}
	}
}

void
RunContext::join()
{
	if (_thread) {
		if (_thread->joinable()) {
			_thread->join();
		}
		_thread.reset();
	}
}

void
RunContext::run()
{
	while (_engine.wait_for_tasks()) {
		for (Task* t = nullptr; (t = _engine.steal_task(0));) {
			t->run(*this);
		}
	}
}

} // namespace server
} // namespace ingen
