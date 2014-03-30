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
	, _head(NULL)
	, _tail(NULL)
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

	/* Note that tail is only used here, not in process().  The head must be
	   checked first here, since if it is NULL the tail pointer is junk. */
	if (!_head) {
		_tail = last;
		_head = first;
	} else {
		_tail.load()->next(first);
		_tail = last;
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

	Event* ev = _head.load();
	if (!ev) {
		// Process audio thread notifications until end
		_engine.process_context().emit_notifications(end_time);
		return;
	}

	while (ev && ev->time() < end_time) {
		Event* const next = (Event*)ev->next();

		// Process audio thread notifications up until this event's time
		_engine.process_context().emit_notifications(ev->time());

		// Process and delete this event
		ev->post_process();
		delete ev;

		ev = next;
	}

	// Since _head was not NULL, we know it hasn't been changed since
	_head = ev;

	/* If next is NULL, then _tail may now be invalid.  However, it would cause
	   a race to reset _tail here.  Instead, append() checks only _head for
	   emptiness, and resets the tail appropriately. */
	
	// Process remaining audio thread notifications until end
	_engine.process_context().emit_notifications(end_time);
}

} // namespace Server
} // namespace Ingen
