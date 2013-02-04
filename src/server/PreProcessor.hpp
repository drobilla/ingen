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

#ifndef INGEN_ENGINE_PREPROCESSOR_HPP
#define INGEN_ENGINE_PREPROCESSOR_HPP

#include <atomic>
#include <thread>
#include <mutex>

#include "raul/Semaphore.hpp"

namespace Ingen {
namespace Server {

class Event;
class PostProcessor;
class ProcessContext;

class PreProcessor
{
public:
	explicit PreProcessor();

	~PreProcessor();

	/** Return true iff no events are enqueued. */
	inline bool empty() const { return !_head.load(); }

	/** Enqueue an event.
	 * This is safe to call from any non-realtime thread (it locks).
	 */
	void event(Event* ev);

	/** Process events for a cycle.
	 * @return The number of events processed.
	 */
	unsigned process(ProcessContext& context,
	                 PostProcessor&  dest,
	                 bool            limit = true);

protected:
	void run();

private:
	std::mutex          _mutex;
	Raul::Semaphore     _sem;
	std::atomic<Event*> _head;
	std::atomic<Event*> _prepared_back;
	std::atomic<Event*> _tail;
	bool                _exit_flag;
	std::thread         _thread;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_PREPROCESSOR_HPP

