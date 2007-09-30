/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include <cassert>
#include <iostream>
#include <pthread.h>
#include <raul/SRSWQueue.hpp>
//#include <raul/Maid.hpp>
#include "events/SendPortValueEvent.hpp"
#include "Event.hpp"
#include "PostProcessor.hpp"
#include "Engine.hpp"
#include "AudioDriver.hpp"
#include "ProcessContext.hpp"

using namespace std;

namespace Ingen {


PostProcessor::PostProcessor(Engine& engine, size_t queue_size)
    : _engine(engine)
	, _max_time(0)
	, _events(queue_size)
{
}


void
PostProcessor::process()
{
	/* Process any audio thread generated events */
	/* FIXME: process events from all thread if parallel */
	/* TODO: obviously this needs work to be generic */
	SendPortValueEvent ev(_engine, 0, NULL, false, 0, 0.0f);
	while (_engine.audio_driver()->context().event_sink().read_control_change(ev)) {
		ev.post_process();
	}

	FrameTime end_time = _max_time.get();

	/* Process normal events */
	while ( ! _events.empty()) {
		Event* const ev = _events.front();
		if (ev->time() > end_time)
			break;
		_events.pop();
		assert(ev);
		ev->post_process();
        delete ev;
	}
}


} // namespace Ingen
