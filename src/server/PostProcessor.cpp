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

#include <assert.h>

#include "raul/log.hpp"

#include "Driver.hpp"
#include "Engine.hpp"
#include "Notification.hpp"
#include "PostProcessor.hpp"
#include "ProcessContext.hpp"
#include "QueuedEvent.hpp"

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
PostProcessor::append(QueuedEvent* first, QueuedEvent* last)
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
	QueuedEvent* ev = _head.get();
	if (!ev) {
		return;
	}
	
	QueuedEvent* const tail = _tail.get();
	_head = (QueuedEvent*)tail->next();
	while (ev && ev->time() <= end_time) {
		QueuedEvent* const next = (QueuedEvent*)ev->next();
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
