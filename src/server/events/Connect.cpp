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

#include "ingen/Store.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"

#include "ArcImpl.hpp"
#include "Broadcaster.hpp"
#include "Connect.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "InputPort.hpp"
#include "PortImpl.hpp"
#include "PreProcessContext.hpp"
#include "internals/BlockDelay.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {
namespace Events {

Connect::Connect(Engine&           engine,
                 SPtr<Interface>   client,
                 int32_t           id,
                 SampleCount       timestamp,
                 const Raul::Path& tail_path,
                 const Raul::Path& head_path)
	: Event(engine, client, id, timestamp)
	, _tail_path(tail_path)
	, _head_path(head_path)
	, _graph(NULL)
	, _head(NULL)
	, _compiled_graph(NULL)
	, _voices(NULL)
{}

bool
Connect::pre_process(PreProcessContext& ctx)
{
	std::lock_guard<Store::Mutex> lock(_engine.store()->mutex());

	Node* tail = _engine.store()->get(_tail_path);
	if (!tail) {
		return Event::pre_process_done(Status::NOT_FOUND, _tail_path);
	}

	Node* head = _engine.store()->get(_head_path);
	if (!head) {
		return Event::pre_process_done(Status::NOT_FOUND, _head_path);
	}

	PortImpl* tail_output = dynamic_cast<PortImpl*>(tail);
	_head                 = dynamic_cast<InputPort*>(head);
	if (!tail_output || !_head) {
		return Event::pre_process_done(Status::BAD_REQUEST, _head_path);
	}

	BlockImpl* const tail_block = tail_output->parent_block();
	BlockImpl* const head_block = _head->parent_block();
	if (!tail_block || !head_block) {
		return Event::pre_process_done(Status::PARENT_NOT_FOUND, _head_path);
	}

	if (tail_block->parent() != head_block->parent()
	    && tail_block != head_block->parent()
	    && tail_block->parent() != head_block) {
		return Event::pre_process_done(Status::PARENT_DIFFERS, _head_path);
	}

	if (!ArcImpl::can_connect(tail_output, _head)) {
		return Event::pre_process_done(Status::TYPE_MISMATCH, _head_path);
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
		return Event::pre_process_done(Status::EXISTS, _head_path);
	}

	_arc = SPtr<ArcImpl>(new ArcImpl(tail_output, _head));

	/* Need to be careful about graph port arcs here and adding a
	   block's parent as a dependant/provider, or adding a graph as its own
	   provider...
	*/
	if (tail_block != head_block && tail_block->parent() == head_block->parent()) {
		// Connection is between blocks inside a graph, compile graph

		// The tail block is now a dependency (provider) of the head block
		head_block->providers().insert(tail_block);

		if (!dynamic_cast<Internals::BlockDelayNode*>(tail_block)) {
			/* Arcs leaving a delay node are ignored for the purposes of
			   compilation, since the output is from the previous cycle and
			   does not affect execution order.  Otherwise, the head block is
			   now a dependant of the head block. */
			tail_block->dependants().insert(head_block);
		}

		if (ctx.must_compile(_graph)) {
			if (!(_compiled_graph = CompiledGraph::compile(_graph))) {
				head_block->providers().erase(tail_block);
				tail_block->dependants().erase(head_block);
				return Event::pre_process_done(Status::COMPILATION_FAILED);
			}
		}
	}

	_graph->add_arc(_arc);
	_head->increment_num_arcs();

	if (!_head->is_driver_port()) {
		_voices = new Raul::Array<PortImpl::Voice>(_head->poly());
		_head->get_buffers(*_engine.buffer_factory(),
		                   _voices,
		                   _head->poly(),
		                   false);
	}

	tail_output->inherit_neighbour(_head, _tail_remove, _tail_add);
	_head->inherit_neighbour(tail_output, _head_remove, _head_add);

	return Event::pre_process_done(Status::SUCCESS);
}

void
Connect::execute(RunContext& context)
{
	if (_status == Status::SUCCESS) {
		_head->add_arc(context, _arc.get());
		if (!_head->is_driver_port()) {
			_engine.maid()->dispose(_head->set_voices(context, _voices));
		}
		_head->connect_buffers();
		if (_compiled_graph) {
			_compiled_graph = _graph->swap_compiled_graph(_compiled_graph);
		}
	}
}

void
Connect::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS) {
		_engine.broadcaster()->connect(_tail_path, _head_path);
		if (!_tail_remove.empty() || !_tail_add.empty()) {
			_engine.broadcaster()->delta(
				Node::path_to_uri(_tail_path), _tail_remove, _tail_add);
		}
		if (!_tail_remove.empty() || !_tail_add.empty()) {
			_engine.broadcaster()->delta(
				Node::path_to_uri(_tail_path), _tail_remove, _tail_add);
		}
	}

	delete _compiled_graph;
}

void
Connect::undo(Interface& target)
{
	target.disconnect(_tail_path, _head_path);
}

} // namespace Events
} // namespace Server
} // namespace Ingen
