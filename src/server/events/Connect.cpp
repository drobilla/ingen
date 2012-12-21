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

#include "ingen/Store.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"

#include "Broadcaster.hpp"
#include "Connect.hpp"
#include "EdgeImpl.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"
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
	, _graph(NULL)
	, _head(NULL)
	, _compiled_graph(NULL)
	, _buffers(NULL)
{}

bool
Connect::pre_process()
{
	Glib::RWLock::ReaderLock rlock(_engine.store()->lock());

	Node* tail = _engine.store()->get(_tail_path);
	if (!tail) {
		return Event::pre_process_done(NOT_FOUND, _tail_path);
	}
		
	Node* head = _engine.store()->get(_head_path);
	if (!head) {
		return Event::pre_process_done(NOT_FOUND, _head_path);
	}

	OutputPort* tail_output = dynamic_cast<OutputPort*>(tail);
	_head                   = dynamic_cast<InputPort*>(head);
	if (!tail_output || !_head) {
		return Event::pre_process_done(BAD_REQUEST, _head_path);
	}

	BlockImpl* const tail_block = tail_output->parent_block();
	BlockImpl* const head_block = _head->parent_block();
	if (!tail_block || !head_block) {
		return Event::pre_process_done(PARENT_NOT_FOUND, _head_path);
	}

	if (tail_block->parent() != head_block->parent()
	    && tail_block != head_block->parent()
	    && tail_block->parent() != head_block) {
		return Event::pre_process_done(PARENT_DIFFERS, _head_path);
	}

	if (!EdgeImpl::can_connect(tail_output, _head)) {
		return Event::pre_process_done(TYPE_MISMATCH, _head_path);
	}

	if (tail_block->parent_graph() != head_block->parent_graph()) {
		// Edge to a graph port from inside the graph
		assert(tail_block->parent() == head_block || head_block->parent() == tail_block);
		if (tail_block->parent() == head_block) {
			_graph = dynamic_cast<GraphImpl*>(head_block);
		} else {
			_graph = dynamic_cast<GraphImpl*>(tail_block);
		}
	} else if (tail_block == head_block && dynamic_cast<GraphImpl*>(tail_block)) {
		// Edge from a graph input to a graph output (pass through)
		_graph = dynamic_cast<GraphImpl*>(tail_block);
	} else {
		// Normal edge between blocks with the same parent
		_graph = tail_block->parent_graph();
	}

	if (_graph->has_edge(tail_output, _head)) {
		return Event::pre_process_done(EXISTS, _head_path);
	}

	_edge = SharedPtr<EdgeImpl>(new EdgeImpl(tail_output, _head));

	rlock.release();

	{
		Glib::RWLock::ReaderLock wlock(_engine.store()->lock());

		/* Need to be careful about graph port edges here and adding a
		   block's parent as a dependant/provider, or adding a graph as its own
		   provider...
		*/
		if (tail_block != head_block && tail_block->parent() == head_block->parent()) {
			head_block->providers().push_back(tail_block);
			tail_block->dependants().push_back(head_block);
		}

		_graph->add_edge(_edge);
		_head->increment_num_edges();
	}

	_buffers = new Raul::Array<BufferRef>(_head->poly());
	_head->get_buffers(*_engine.buffer_factory(),
	                   _buffers,
	                   _head->poly(),
	                   false);

	if (_graph->enabled()) {
		_compiled_graph = _graph->compile();
	}

	return Event::pre_process_done(SUCCESS);
}

void
Connect::execute(ProcessContext& context)
{
	if (!_status) {
		_head->add_edge(context, _edge.get());
		_engine.maid()->dispose(_head->set_buffers(context, _buffers));
		_head->connect_buffers();
		_graph->set_compiled_graph(_compiled_graph);
	}
}

void
Connect::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (!respond()) {
		_engine.broadcaster()->connect(_tail_path, _head_path);
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
