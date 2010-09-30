/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_MESSAGECONTEXT_HPP
#define INGEN_ENGINE_MESSAGECONTEXT_HPP

#include <set>
#include <glibmm/thread.h>
#include "raul/Thread.hpp"
#include "raul/Semaphore.hpp"
#include "raul/AtomicPtr.hpp"
#include "atom.lv2/atom.h"
#include "Context.hpp"
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"
#include "tuning.hpp"

namespace Ingen {

class PortImpl;

/** Context of a message_run() call.
 *
 * The message context is a non-hard-realtime thread used to execute things
 * that can take too long to execute in an audio thread, and do sloppy timed
 * event propagation and scheduling.  Interface to plugins via the
 * LV2 contexts extension.
 *
 * \ingroup engine
 */
class MessageContext : public Context, public Raul::Thread
{
public:
	explicit MessageContext(Engine& engine)
		: Context(engine, MESSAGE)
		, Raul::Thread("MessageContext")
		, _sem(0)
		, _requests(message_context_queue_size)
		, _end_time(0)
	{
		Thread::set_context(THREAD_MESSAGE);
	}

	/** Schedule a port value change at a certain time.
	 * Safe to call from either process thread or pre-process thread.
	 */
	void run(PortImpl* port, FrameTime time);

protected:
	struct Request {
		Request(FrameTime t=0, PortImpl* p=0) : time(t), port(p) {}
		FrameTime time;
		PortImpl* port;
	};

public:
	/** Signal the end of a cycle that has produced messages.
	 * AUDIO THREAD ONLY.
	 */
	inline void signal(ProcessContext& context) {
		ThreadManager::assert_thread(THREAD_PROCESS);
		const Request cycle_end_request(context.end(), NULL);
		_requests.write(sizeof(Request), &cycle_end_request);
		_sem.post();
	}

	/** Return true iff requests are pending.  Safe from any thread. */
	inline bool has_requests() const {
		return _requests.read_space() >= sizeof(Request);
	}

protected:
	/** Thread run method (wait for and execute requests from process thread */
	void _run();

	/** Execute a request (possibly enqueueing more requests) */
	void execute(const Request& req);

	Raul::Semaphore           _sem;
	Raul::RingBuffer<Request> _requests;
	Glib::Mutex               _mutex;
	Glib::Cond                _cond;
	Request                   _non_rt_request;

	struct RequestEarlier {
		bool operator()(const Request& r1, const Request& r2) {
			return r1.time < r2.time;
		}
	};

	typedef std::set<Request, RequestEarlier> Queue;
	Queue     _queue;
	FrameTime _end_time;
};


} // namespace Ingen

#endif // INGEN_ENGINE_MESSAGECONTEXT_HPP

