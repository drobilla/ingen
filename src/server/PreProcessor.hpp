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

#ifndef INGEN_ENGINE_PREPROCESSOR_HPP
#define INGEN_ENGINE_PREPROCESSOR_HPP

#include "Event.hpp"

#include "raul/Semaphore.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <thread>

namespace ingen {
namespace server {

class Engine;
class PostProcessor;
class RunContext;

class PreProcessor
{
public:
	explicit PreProcessor(Engine& engine);

	~PreProcessor();

	/** Return true iff no events are enqueued. */
	inline bool empty() const { return !_head.load(); }

	/** Enqueue an event.
	 * This is safe to call from any non-realtime thread (it locks).
	 */
	void event(Event* ev, Event::Mode mode);

	/** Process events for a cycle.
	 * @return The number of events processed.
	 */
	unsigned process(RunContext&    context,
	                 PostProcessor& dest,
	                 size_t         limit = 0);

protected:
	void run();

private:
	enum class BlockState {
		UNBLOCKED,      ///< Normal, unblocked execution
		PRE_BLOCKED,    ///< Preprocess thread has enqueued blocking event
		BLOCKED,        ///< Process thread has reached blocking event
		PRE_UNBLOCKED,  ///< Preprocess thread has enqueued unblocking event
		PROCESSING      ///< Process thread is executing all events in-between
	};

	void wait_for_block_state(const BlockState state) {
		while (_block_state != state) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	Engine&                 _engine;
	std::mutex              _mutex;
	Raul::Semaphore         _sem;
	std::atomic<Event*>     _head;
	std::atomic<Event*>     _tail;
	std::atomic<BlockState> _block_state;
	bool                    _exit_flag;
	std::thread             _thread;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_PREPROCESSOR_HPP
