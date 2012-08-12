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

#include <glibmm/thread.h>

#include "raul/Maid.hpp"
#include "raul/Path.hpp"

#include "Broadcaster.hpp"
#include "Connect.hpp"
#include "EdgeImpl.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"
#include "PatchImpl.hpp"
#include "PortImpl.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {
namespace Events {

Connect::Connect(Engine&              engine,
                 SharedPtr<Interface> client,
                 int32_t              id,
                 SampleCount          timestamp,
                 const Raul::Path&    tail_path,
                 const Raul::Path&    head_path)
	: Event(engine, client, id, timestamp)
	, _tail_path(tail_path)
	, _head_path(head_path)
	, _patch(NULL)
	, _head(NULL)
	, _compiled_patch(NULL)
	, _buffers(NULL)
{}

bool
Connect::pre_process()
{
	Glib::RWLock::ReaderLock rlock(_engine.engine_store()->lock());

	PortImpl* tail = _engine.engine_store()->find_port(_tail_path);
	PortImpl* head = _engine.engine_store()->find_port(_head_path);
	if (!tail) {
		return Event::pre_process_done(PORT_NOT_FOUND, _tail_path.str());
	} else if (!head) {
		return Event::pre_process_done(PORT_NOT_FOUND, _head_path.str());
	}

	OutputPort* tail_output = dynamic_cast<OutputPort*>(tail);
	_head                   = dynamic_cast<InputPort*>(head);
	if (!tail_output || !_head) {
		return Event::pre_process_done(DIRECTION_MISMATCH, _head_path.str());
	}

	NodeImpl* const tail_node = tail->parent_node();
	NodeImpl* const head_node = head->parent_node();
	if (!tail_node || !head_node) {
		return Event::pre_process_done(PARENT_NOT_FOUND, _head_path.str());
	}

	if (tail_node->parent() != head_node->parent()
	    && tail_node != head_node->parent()
	    && tail_node->parent() != head_node) {
		return Event::pre_process_done(PARENT_DIFFERS, _head_path.str());
	}

	if (!EdgeImpl::can_connect(tail_output, _head)) {
		return Event::pre_process_done(TYPE_MISMATCH, _head_path);
	}

	if (tail_node->parent_patch() != head_node->parent_patch()) {
		// Edge to a patch port from inside the patch
		assert(tail_node->parent() == head_node || head_node->parent() == tail_node);
		if (tail_node->parent() == head_node) {
			_patch = dynamic_cast<PatchImpl*>(head_node);
		} else {
			_patch = dynamic_cast<PatchImpl*>(tail_node);
		}
	} else if (tail_node == head_node && dynamic_cast<PatchImpl*>(tail_node)) {
		// Edge from a patch input to a patch output (pass through)
		_patch = dynamic_cast<PatchImpl*>(tail_node);
	} else {
		// Normal edge between nodes with the same parent
		_patch = tail_node->parent_patch();
	}

	if (_patch->has_edge(tail_output, _head)) {
		return Event::pre_process_done(EXISTS, _head_path);
	}

	_edge = SharedPtr<EdgeImpl>(new EdgeImpl(tail_output, _head));

	rlock.release();

	{
		Glib::RWLock::ReaderLock wlock(_engine.engine_store()->lock());

		/* Need to be careful about patch port edges here and adding a
		   node's parent as a dependant/provider, or adding a patch as its own
		   provider...
		*/
		if (tail_node != head_node && tail_node->parent() == head_node->parent()) {
			head_node->providers().push_back(tail_node);
			tail_node->dependants().push_back(head_node);
		}

		_patch->add_edge(_edge);
		_head->increment_num_edges();
	}

	_buffers = new Raul::Array<BufferRef>(_head->poly());
	_head->get_buffers(*_engine.buffer_factory(),
	                   _buffers,
	                   _head->poly(),
	                   false);

	if (_patch->enabled()) {
		_compiled_patch = _patch->compile();
	}

	return Event::pre_process_done(SUCCESS);
}

void
Connect::execute(ProcessContext& context)
{
	if (!_status) {
		_head->add_edge(context, _edge.get());
		_engine.maid()->push(_head->set_buffers(context, _buffers));
		_head->connect_buffers();
		_engine.maid()->push(_patch->compiled_patch());
		_patch->compiled_patch(_compiled_patch);
	}
}

void
Connect::post_process()
{
	if (!respond()) {
		_engine.broadcaster()->connect(_tail_path, _head_path);
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
