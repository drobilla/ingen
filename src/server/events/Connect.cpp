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

#include "Connect.hpp"

#include "ArcImpl.hpp"
#include "BlockImpl.hpp"
#include "Broadcaster.hpp"
#include "BufferFactory.hpp"
#include "CompiledGraph.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "InputPort.hpp"
#include "PortImpl.hpp"
#include "PreProcessContext.hpp"
#include "internals/BlockDelay.hpp"
#include "types.hpp"

#include "ingen/Interface.hpp"
#include "ingen/Node.hpp"
#include "ingen/Status.hpp"
#include "ingen/Store.hpp"
#include "ingen/paths.hpp"
#include "raul/Maid.hpp"

#include <cassert>
#include <memory>
#include <mutex>
#include <set>
#include <utility>

namespace ingen::server::events {

Connect::Connect(Engine&                           engine,
                 const std::shared_ptr<Interface>& client,
                 SampleCount                       timestamp,
                 const ingen::Connect&             msg)
    : Event(engine, client, msg.seq, timestamp)
    , _msg(msg)
{}

Connect::~Connect() = default;

bool
Connect::pre_process(PreProcessContext& ctx)
{
	std::lock_guard<Store::Mutex> lock(_engine.store()->mutex());

	Node* tail = _engine.store()->get(_msg.tail);
	if (!tail) {
		return Event::pre_process_done(Status::NOT_FOUND, _msg.tail);
	}

	Node* head = _engine.store()->get(_msg.head);
	if (!head) {
		return Event::pre_process_done(Status::NOT_FOUND, _msg.head);
	}

	auto* tail_output = dynamic_cast<PortImpl*>(tail);
	_head             = dynamic_cast<InputPort*>(head);
	if (!tail_output || !_head) {
		return Event::pre_process_done(Status::BAD_REQUEST, _msg.head);
	}

	BlockImpl* const tail_block = tail_output->parent_block();
	BlockImpl* const head_block = _head->parent_block();
	if (!tail_block || !head_block) {
		return Event::pre_process_done(Status::PARENT_NOT_FOUND, _msg.head);
	}

	if (tail_block->parent() != head_block->parent()
	    && tail_block != head_block->parent()
	    && tail_block->parent() != head_block) {
		return Event::pre_process_done(Status::PARENT_DIFFERS, _msg.head);
	}

	if (!ArcImpl::can_connect(tail_output, _head)) {
		return Event::pre_process_done(Status::TYPE_MISMATCH, _msg.head);
	}

	if (tail_block->parent_graph() != head_block->parent_graph()) {
		// Arc to a graph port from inside the graph
		assert(tail_block->parent() == head_block || head_block->parent() == tail_block);
		if (tail_block->parent() == head_block) {
			_graph = dynamic_cast<GraphImpl*>(head_block);
		} else {
			_graph = dynamic_cast<GraphImpl*>(tail_block);
		}
	} else if (tail_block == head_block && dynamic_cast<GraphImpl*>(tail_block)) {
		// Arc from a graph input to a graph output (pass through)
		_graph = dynamic_cast<GraphImpl*>(tail_block);
	} else {
		// Normal arc between blocks with the same parent
		_graph = tail_block->parent_graph();
	}

	if (_graph->has_arc(tail_output, _head)) {
		return Event::pre_process_done(Status::EXISTS, _msg.head);
	}

	_arc = std::make_shared<ArcImpl>(tail_output, _head);

	/* Need to be careful about graph port arcs here and adding a
	   block's parent as a dependant/provider, or adding a graph as its own
	   provider...
	*/
	if (tail_block != head_block && tail_block->parent() == head_block->parent()) {
		// Connection is between blocks inside a graph, compile graph

		if (!dynamic_cast<internals::BlockDelayNode*>(tail_block)) {
			/* Arcs leaving a delay node are ignored for the purposes of
			   compilation, since the output is from the previous cycle and
			   does not affect execution order.  Otherwise, the head block is
			   now a dependant of the head block. */
			tail_block->dependants().insert(head_block);
		}

		// The tail block is now a dependency (provider) of the head block
		head_block->providers().insert(tail_block);

		if (ctx.must_compile(*_graph)) {
			if (!(_compiled_graph = compile(*_engine.maid(), *_graph))) {
				head_block->providers().erase(tail_block);
				tail_block->dependants().erase(head_block);
				return Event::pre_process_done(Status::COMPILATION_FAILED);
			}
		}
	}

	_graph->add_arc(_arc);
	_head->increment_num_arcs();

	if (!_head->is_driver_port()) {
		BufferFactory& bufs = *_engine.buffer_factory();
		_voices = bufs.maid().make_managed<PortImpl::Voices>(_head->poly());
		_head->pre_get_buffers(bufs, _voices, _head->poly());
	}

	tail_output->inherit_neighbour(_head, _tail_remove, _tail_add);
	_head->inherit_neighbour(tail_output, _head_remove, _head_add);

	return Event::pre_process_done(Status::SUCCESS);
}

void
Connect::execute(RunContext& ctx)
{
	if (_status == Status::SUCCESS) {
		_head->add_arc(ctx, *_arc);
		if (!_head->is_driver_port()) {
			_head->set_voices(ctx, std::move(_voices));
		}
		_head->connect_buffers();
		if (_compiled_graph) {
			_graph->set_compiled_graph(std::move(_compiled_graph));
		}
	}
}

void
Connect::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS) {
		_engine.broadcaster()->message(_msg);
		if (!_tail_remove.empty() || !_tail_add.empty()) {
			_engine.broadcaster()->delta(
				path_to_uri(_msg.tail), _tail_remove, _tail_add);
		}
		if (!_tail_remove.empty() || !_tail_add.empty()) {
			_engine.broadcaster()->delta(
				path_to_uri(_msg.tail), _tail_remove, _tail_add);
		}
	}
}

void
Connect::undo(Interface& target)
{
	target.disconnect(_msg.tail, _msg.head);
}

} // namespace ingen::server::events
