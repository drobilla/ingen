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

class Sentinel : public Event {
public:
	Sentinel(Engine& engine) : Event(engine) {}

	bool pre_process() { return false; }
	void execute(ProcessContext& context) {}
	void post_process() {}
};

PostProcessor::PostProcessor(Engine& engine)
	: _engine(engine)
	, _head(new Sentinel(engine))
	, _tail(_head.load())
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
	assert(!last->next());

	// The only place where _tail is written or next links are changed
	_tail.load()->next(first);
	_tail.store(last);
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

	/* We can never empty the list and set _head = _tail = NULL since this
	   would cause a race with append.  Instead, head is an already
	   post-processed node, or initially a sentinel. */
	Event* ev   = _head.load();
	Event* next = (Event*)ev->next();
	if (!next || next->time() >= end_time) {
		// Process audio thread notifications until end
		_engine.process_context().emit_notifications(end_time);
		return;
	}

	do {
		// Delete previously post-processed ev and move to next
		delete ev;
		ev = next;

		// Process audio thread notifications up until this event's time
		_engine.process_context().emit_notifications(ev->time());

		// Post-process event
		ev->post_process();
		next = ev->next();  // [1] (see below)
	} while (next && next->time() < end_time);

	/* Reached the tail (as far as we're concerned).  There may be successors
	   by now if append() has been called since [1], but that's fine.  Now, ev
	   points to the last post-processed event, which will be the new head. */
	assert(ev);
	_head = ev;
	
	// Process remaining audio thread notifications until end
	_engine.process_context().emit_notifications(end_time);
}

} // namespace Server
} // namespace Ingen
