/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_POSTPROCESSOR_HPP
#define INGEN_ENGINE_POSTPROCESSOR_HPP

#include "server.h"
#include "types.hpp"

#include <atomic>

namespace ingen {
namespace server {

class Engine;
class Event;
class RunContext;

/** Processor for Events after leaving the audio thread.
 *
 * The audio thread pushes events to this when it is done with them (which
 * is realtime-safe), which signals the processing thread through a semaphore
 * to handle the event and pass it on to the Maid.
 *
 * Update: This is all run from main_iteration now to solve scripting
 * thread issues.  Not sure if this is permanent/ideal or not...
 *
 * \ingroup engine
 */
class INGEN_SERVER_API PostProcessor
{
public:
	explicit PostProcessor(Engine& engine);
	~PostProcessor();

	/** Push a list of events on to the process queue.
	    realtime-safe, not thread-safe.
	*/
	void append(RunContext& ctx, Event* first, Event* last);

	/** Post-process and delete all pending events */
	void process();

	/** Return true iff any events are pending */
	bool pending() const;

	/** Set the latest event time that should be post-processed */
	void set_end_time(FrameTime time) { _max_time = time; }

private:
	Engine&                _engine;
	std::atomic<Event*>    _head;
	std::atomic<Event*>    _tail;
	std::atomic<FrameTime> _max_time;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_POSTPROCESSOR_HPP
