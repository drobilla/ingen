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

using namespace std;

namespace Ingen {

void
MessageContext::run(NodeImpl* node)
{
	node->message_run(*this);

	void*      valid_ports = node->valid_ports();
	PatchImpl* patch       = node->parent_patch();

	//cout << "MESSAGE RUN " << node->path() << " {" << endl;
	for (uint32_t i = 0; i < node->num_ports(); ++i) {
		PortImpl* p = node->port_impl(i);
		if (p->is_output() && p->context() == Context::MESSAGE &&
				lv2_contexts_port_is_valid(valid_ports, i)) {
			PatchImpl::Connections& wires = patch->connections();
			for (PatchImpl::Connections::iterator c = wires.begin(); c != wires.end(); ++c) {
				ConnectionImpl* ci = dynamic_cast<ConnectionImpl*>(c->get());
				if (ci->src_port() == p) {
					ci->dst_port()->pre_process(*_engine.message_context());
					run(ci->dst_port()->parent_node());
				}
			}
		}
	}
	//cout << "}" << endl;

	node->reset_valid_ports();
}

} // namespace Ingen
