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
#include "PostProcessor.hpp"
#include "ProcessContext.hpp"
#include "QueuedEvent.hpp"
#include "events/SendPortValue.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

PostProcessor::PostProcessor(Engine& engine)
    : _engine(engine)
	, _max_time(0)
	, _event_buffer_size(sizeof(Events::SendPortValue)) // FIXME: make generic
	, _event_buffer((uint8_t*)malloc(_event_buffer_size))
{
}

PostProcessor::~PostProcessor()
{
	free(_event_buffer);
}

void
PostProcessor::append(QueuedEvent* first, QueuedEvent* last)
{
	assert(first);
	assert(last);
	QueuedEvent* const head = _head.get();
	QueuedEvent* const tail = _tail.get();
	if (!head) {
		_head = first;
	} else {
		tail->next(first);
	}
	_tail = last;
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
	while (true) {
		Driver* driver = _engine.driver();
		if (driver && driver->context().event_sink().read(_event_buffer_size, _event_buffer)) {
			if (((Event*)_event_buffer)->time() > end_time) {
				warn << "Lost event with time "
					<< ((Event*)_event_buffer)->time() << " > " << end_time << endl;
				break;
			}
			((Event*)_event_buffer)->post_process();
		} else {
			break;
		}
	}

	/* Process normal events */
	QueuedEvent* ev = _head.get();
	while (ev) {
		if (ev->time() > end_time)
			break;

		QueuedEvent* const next = (QueuedEvent*)ev->next();
		ev->post_process();
		if (next) {
			_head = next;
		} else {
			_head = NULL;
			_tail = NULL;
		}
		delete ev;
		ev = next;
	}
}

} // namespace Server
} // namespace Ingen
