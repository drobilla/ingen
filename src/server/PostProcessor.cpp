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


#include <assert.h>

#include "raul/log.hpp"

#include "Driver.hpp"
#include "Engine.hpp"
#include "Notification.hpp"
#include "PostProcessor.hpp"
#include "ProcessContext.hpp"
#include "Event.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

PostProcessor::PostProcessor(Engine& engine)
    : _engine(engine)
	, _max_time(0)
{
}

PostProcessor::~PostProcessor()
{
}

void
PostProcessor::append(Event* first, Event* last)
{
	assert(first);
	assert(last);
	assert(!last->next());
	if (_head.get()) {
		_tail.get()->next(first);
		_tail = last;
	} else {
		_tail = last;
		_head = first;
	}
}

void
PostProcessor::process()
{
	const FrameTime end_time = _max_time.get();

	/* FIXME: The order here is a bit tricky: if the normal events are
	 * processed first, then a deleted port may still have a pending
	 * broadcast event which will segfault if executed afterwards.
	 * If it's the other way around, broadcasts will be sent for
	 * ports the client doesn't even know about yet... */

	/* FIXME: process events from all threads if parallel */

	/* Process audio thread generated events */
	Driver* driver = _engine.driver();
	if (driver) {
		Raul::RingBuffer& event_sink = driver->context().event_sink();
		const uint32_t    read_space = event_sink.read_space();
		Notification      note;
		for (uint32_t i = 0; i < read_space; i += sizeof(note)) {
			if (event_sink.read(sizeof(note), &note) == sizeof(note)) {
				Notification::post_process(note, _engine);
			}
		}
	}

	/* Process normal events */
	Event* ev = _head.get();
	if (!ev) {
		return;
	}

	Event* const tail = _tail.get();
	_head = (Event*)tail->next();
	while (ev && ev->time() <= end_time) {
		Event* const next = (Event*)ev->next();
		ev->post_process();
		delete ev;
		if (ev == tail) {
			break;
		}
		ev = next;
	}
}

} // namespace Server
} // namespace Ingen
