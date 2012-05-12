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

#include <string>

#include <boost/format.hpp>
#include <glibmm/thread.h>

#include "raul/Maid.hpp"
#include "raul/Path.hpp"

#include "ClientBroadcaster.hpp"
#include "Connect.hpp"
#include "EdgeImpl.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"
#include "PatchImpl.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {
namespace Events {

Connect::Connect(Engine&           engine,
                 Interface*        client,
                 int32_t           id,
                 SampleCount       timestamp,
                 const Raul::Path& tail_path,
                 const Raul::Path& head_path)
	: Event(engine, client, id, timestamp)
	, _tail_path(tail_path)
	, _head_path(head_path)
	, _patch(NULL)
	, _src_output_port(NULL)
	, _dst_input_port(NULL)
	, _compiled_patch(NULL)
	, _buffers(NULL)
{}

void
Connect::pre_process()
{
	Glib::RWLock::ReaderLock rlock(_engine.engine_store()->lock());

	PortImpl* tail = _engine.engine_store()->find_port(_tail_path);
	PortImpl* head = _engine.engine_store()->find_port(_head_path);
	if (!tail || !head) {
		_status = PORT_NOT_FOUND;
		Event::pre_process();
		return;
	}

	_dst_input_port  = dynamic_cast<InputPort*>(head);
	_src_output_port = dynamic_cast<OutputPort*>(tail);
	if (!_dst_input_port || !_src_output_port) {
		_status = DIRECTION_MISMATCH;
		Event::pre_process();
		return;
	}

	NodeImpl* const src_node = tail->parent_node();
	NodeImpl* const dst_node = head->parent_node();
	if (!src_node || !dst_node) {
		_status = PARENT_NOT_FOUND;
		Event::pre_process();
		return;
	}

	if (src_node->parent() != dst_node->parent()
			&& src_node != dst_node->parent()
			&& src_node->parent() != dst_node) {
		_status = PARENT_DIFFERS;
		Event::pre_process();
		return;
	}

	if (!EdgeImpl::can_connect(_src_output_port, _dst_input_port)) {
		_status = TYPE_MISMATCH;
		Event::pre_process();
		return;
	}

	// Edge to a patch port from inside the patch
	if (src_node->parent_patch() != dst_node->parent_patch()) {
		assert(src_node->parent() == dst_node || dst_node->parent() == src_node);
		if (src_node->parent() == dst_node)
			_patch = dynamic_cast<PatchImpl*>(dst_node);
		else
			_patch = dynamic_cast<PatchImpl*>(src_node);

	// Edge from a patch input to a patch output (pass through)
	} else if (src_node == dst_node && dynamic_cast<PatchImpl*>(src_node)) {
		_patch = dynamic_cast<PatchImpl*>(src_node);

	// Normal edge between nodes with the same parent
	} else {
		_patch = src_node->parent_patch();
	}

	if (_patch->has_edge(_src_output_port, _dst_input_port)) {
		_status = EXISTS;
		Event::pre_process();
		return;
	}

	_edge = SharedPtr<EdgeImpl>(
		new EdgeImpl(_src_output_port, _dst_input_port));

	rlock.release();

	{
		Glib::RWLock::ReaderLock wlock(_engine.engine_store()->lock());

		/* Need to be careful about patch port edges here and adding a
		   node's parent as a dependant/provider, or adding a patch as its own
		   provider...
		*/
		if (src_node != dst_node && src_node->parent() == dst_node->parent()) {
			dst_node->providers().push_back(src_node);
			src_node->dependants().push_back(dst_node);
		}

		_patch->add_edge(_edge);
		_dst_input_port->increment_num_edges();
	}

	_buffers = new Raul::Array<BufferRef>(_dst_input_port->poly());
	_dst_input_port->get_buffers(_engine.message_context(),
	                             *_engine.buffer_factory(),
	                             _buffers, _dst_input_port->poly());

	if (_patch->enabled())
		_compiled_patch = _patch->compile();

	Event::pre_process();
}

void
Connect::execute(ProcessContext& context)
{
	Event::execute(context);

	if (_status == SUCCESS) {
		// This must be inserted here, since they're actually used by the audio thread
		_dst_input_port->add_edge(context, _edge.get());
		assert(_buffers);
		_engine.maid()->push(_dst_input_port->set_buffers(context, _buffers));
		_dst_input_port->connect_buffers();
		_engine.maid()->push(_patch->compiled_patch());
		_patch->compiled_patch(_compiled_patch);
	}
}

void
Connect::post_process()
{
	respond(_status);
	if (!_status) {
		_engine.broadcaster()->connect(_tail_path, _head_path);
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
