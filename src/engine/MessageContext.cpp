/* This file is part of Ingen.
 * Copyright (C) 2008-2009 Dave Robillard <http://drobilla.net>
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

#include <iostream>
#include "raul/Atom.hpp"
#include "ConnectionImpl.hpp"
#include "Engine.hpp"
#include "MessageContext.hpp"
#include "NodeImpl.hpp"
#include "PatchImpl.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"

using namespace std;

namespace Ingen {


void
MessageContext::run(NodeImpl* node)
{
	if (ThreadManager::current_thread_id() == THREAD_PRE_PROCESS) {
		assert(node);
		Glib::Mutex::Lock lock(_mutex);
		_request = node;
		_sem.post();
		_cond.wait(_mutex);
	} else if (ThreadManager::current_thread_id() == THREAD_PROCESS) {
		_requests.write(sizeof(NodeImpl*), &node);
		// signal() will be called at the end of this process cycle
	} else if (ThreadManager::current_thread_id() == THREAD_MESSAGE) {
		cout << "Message context recursion at " << node->path() << endl;
	} else {
		cout << "[MessageContext] ERROR: Run requested from unknown thread" << endl;
	}
}


void
MessageContext::_run()
{
	NodeImpl* node = NULL;

	while (true) {
		_sem.wait();

		// Run a node requested by the pre-process thread
		{
			Glib::Mutex::Lock lock(_mutex);
			node = _request.get();
			if (node) {
				_cond.broadcast(); // Notify caller we got the message
				run_node(node);
			}
		}

		// Run nodes requested by the audio thread
		while (has_requests()) {
			_requests.full_read(sizeof(NodeImpl*), &node);
			run_node(node);
		}
	}
}


void
MessageContext::run_node(NodeImpl* node)
{
	node->message_run(*this);

	void*      valid_ports = node->valid_ports();
	PatchImpl* patch       = node->parent_patch();

	for (uint32_t i = 0; i < node->num_ports(); ++i) {
		PortImpl* p = node->port_impl(i);
		if (p->is_output() && p->context() == Context::MESSAGE &&
				lv2_contexts_port_is_valid(valid_ports, i)) {
			PatchImpl::Connections& wires = patch->connections();
			for (PatchImpl::Connections::iterator c = wires.begin(); c != wires.end(); ++c) {
				ConnectionImpl* ci = dynamic_cast<ConnectionImpl*>(c->get());
				if (ci->src_port() == p) {
					ci->dst_port()->pre_process(*_engine.message_context());
					run_node(ci->dst_port()->parent_node());
				}
			}
		}
	}

	node->reset_valid_ports();
}

} // namespace Ingen
