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

#include <algorithm>
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "raul/log.hpp"
#include "ConnectionImpl.hpp"
#include "Engine.hpp"
#include "MessageContext.hpp"
#include "NodeImpl.hpp"
#include "PatchImpl.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

void
MessageContext::run(NodeImpl* node, FrameTime time)
{
	if (ThreadManager::thread_is(THREAD_PROCESS)) {
		const Request r(time, node);
		_requests.write(sizeof(Request), &r);
		// signal() will be called at the end of this process cycle
	} else {
		assert(node);
		Glib::Mutex::Lock lock(_mutex);
		_non_rt_request = Request(time, node);
		_sem.post();
		_cond.wait(_mutex);
	}
}

void
MessageContext::_run()
{
	Request req;

	while (true) {
		_sem.wait();

		// Enqueue a request from the pre-process thread
		{
			Glib::Mutex::Lock lock(_mutex);
			const Request req = _non_rt_request;
			if (req.node) {
				_queue.insert(req);
				_end_time = std::max(_end_time, req.time);
				_cond.broadcast(); // Notify caller we got the message
			}
		}

		// Enqueue (and thereby sort) requests from audio thread
		while (has_requests()) {
			_requests.read(sizeof(Request), &req);
			if (req.node) {
				_queue.insert(req);
			} else {
				_end_time = req.time;
				break;
			}
		}

		// Run events in time increasing order
		// Note that executing nodes may insert further events into the queue
		while (!_queue.empty()) {
			const Request req = *_queue.begin();

			// Break if all events during this cycle have been consumed
			// (the queue may contain generated events with later times)
			if (req.time > _end_time) {
				break;
			}

			_queue.erase(_queue.begin());
			execute(req);
		}
	}
}

void
MessageContext::execute(const Request& req)
{
	NodeImpl* node = req.node;
	node->work(*this, 0, NULL);
}

} // namespace Server
} // namespace Ingen
