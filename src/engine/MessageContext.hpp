/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef MESSAGECONTEXT_H
#define MESSAGECONTEXT_H

#include <glibmm/thread.h>
#include "raul/Thread.hpp"
#include "raul/Semaphore.hpp"
#include "raul/AtomicPtr.hpp"
#include "object.lv2/object.h"
#include "Context.hpp"
#include "ThreadManager.hpp"
#include "tuning.hpp"

namespace Ingen {

class NodeImpl;

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
	MessageContext(Engine& engine)
		: Context(engine, MESSAGE)
		, Raul::Thread("message-context")
		, _sem(0)
		, _requests(message_context_queue_size)
	{
		Thread::set_context(THREAD_MESSAGE);
	}

	/** Request a run starting at node.
	 *
	 * Safe to call from either process thread or pre-process thread.
	 */
	void run(NodeImpl* node);

	inline void signal() { _sem.post(); }
	inline bool has_requests() const {
		return _requests.read_space() >= sizeof(NodeImpl*) || _request.get();
	}

protected:
	/** Thread run method (wait for and execute requests from process thread */
	void _run();

	/** Actually execute and propagate from node */
	void run_node(NodeImpl* node);

	Raul::Semaphore             _sem;
	Raul::RingBuffer<NodeImpl*> _requests;
	Glib::Mutex                 _mutex;
	Glib::Cond                  _cond;
	Raul::AtomicPtr<NodeImpl>   _request;
};


} // namespace Ingen

#endif // MESSAGECONTEXT_H

