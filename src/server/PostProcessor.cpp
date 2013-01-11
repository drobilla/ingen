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

#include "Engine.hpp"
#include "Event.hpp"
#include "PostProcessor.hpp"
#include "ProcessContext.hpp"

using namespace std;

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
PostProcessor::append(ProcessContext& context, Event* first, Event* last)
{
	assert(first);
	assert(last);
	if (_head) {
		_tail.load()->next(first);
		_tail = last;
	} else {
		_tail = last;
		_head = first;
	}
}

bool
PostProcessor::pending() const
{
	return _head.load() || _engine.process_context().pending_notifications();
}

void
PostProcessor::process()
{
	const FrameTime end_time = _max_time;

	// To avoid a race, we only process up to tail and never write to _tail
	Event* const tail = _tail.load();

	Event* ev = _head.load();
	if (!ev) {
		// Process audio thread notifications up until end
		_engine.process_context().emit_notifications(end_time);
		return;
	}

	while (ev->time() < end_time) {
		Event* const next = (Event*)ev->next();

		// Process audio thread notifications up until this event's time
		_engine.process_context().emit_notifications(ev->time());

		// Process and delete this event
		ev->post_process();
		delete ev;

		if (ev == tail || (next && next->time() >= end_time)) {
			// Reached end, update _head
			_head = next;
			break;
		}

		ev = next;
	}
}

} // namespace Server
} // namespace Ingen
