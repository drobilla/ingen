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

#include <set>

#include "ingen/Store.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"

#include "ArcImpl.hpp"
#include "Broadcaster.hpp"
#include "Buffer.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "InputPort.hpp"
#include "PortImpl.hpp"
#include "PreProcessContext.hpp"
#include "RunContext.hpp"
#include "ThreadManager.hpp"
#include "events/Disconnect.hpp"

namespace ingen {
namespace server {
namespace events {

Disconnect::Disconnect(Engine&                  engine,
                       SPtr<Interface>          client,
                       SampleCount              timestamp,
                       const ingen::Disconnect& msg)
	: Event(engine, client, msg.seq, timestamp)
	, _msg(msg)
	, _graph(nullptr)
{
}

Disconnect::Impl::Impl(Engine&     e,
                       GraphImpl*  graph,
                       PortImpl*   t,
                       InputPort*  h)
	: _engine(e)
	, _tail(t)
	, _head(h)
	, _arc(graph->remove_arc(_tail, _head))
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	BlockImpl* const tail_block = _tail->parent_block();
	BlockImpl* const head_block = _head->parent_block();

	// Remove tail from head's providers
	auto hp = head_block->providers().find(tail_block);
	if (hp != head_block->providers().end()) {
		head_block->providers().erase(hp);
	}

	// Remove head from tail's providers
	auto td = tail_block->dependants().find(head_block);
	if (td != tail_block->dependants().end()) {
		tail_block->dependants().erase(td);
	}

	_head->decrement_num_arcs();

	if (_head->num_arcs() == 0) {
		if (!_head->is_driver_port()) {
			BufferFactory& bufs = *_engine.buffer_factory();
			_voices = bufs.maid().make_managed<PortImpl::Voices>(_head->poly());
			_head->pre_get_buffers(bufs, _voices, _head->poly());

			if (_head->is_a(PortType::CONTROL) ||
			    _head->is_a(PortType::CV)) {
				// Reset buffer to control value
				const float value = _head->value().get<float>();
				for (uint32_t i = 0; i < _voices->size(); ++i) {
					Buffer* buf = _voices->at(i).buffer.get();
					buf->set_block(value, 0, e.block_length());
				}
			} else {
				for (uint32_t i = 0; i < _voices->size(); ++i) {
					_voices->at(i).buffer->clear();
				}
			}
		}
	}
}

bool
Disconnect::pre_process(PreProcessContext& ctx)
{
	std::lock_guard<Store::Mutex> lock(_engine.store()->mutex());

	if (_msg.tail.parent().parent() != _msg.head.parent().parent()
	    && _msg.tail.parent() != _msg.head.parent().parent()
	    && _msg.tail.parent().parent() != _msg.head.parent()) {
		return Event::pre_process_done(Status::PARENT_DIFFERS, _msg.head);
	}

	PortImpl* tail = dynamic_cast<PortImpl*>(_engine.store()->get(_msg.tail));
	if (!tail) {
		return Event::pre_process_done(Status::PORT_NOT_FOUND, _msg.tail);
	}

	PortImpl* head = dynamic_cast<PortImpl*>(_engine.store()->get(_msg.head));
	if (!head) {
		return Event::pre_process_done(Status::PORT_NOT_FOUND, _msg.head);
	}

	BlockImpl* const tail_block = tail->parent_block();
	BlockImpl* const head_block = head->parent_block();

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

	if (!_graph) {
		return Event::pre_process_done(Status::INTERNAL_ERROR, _msg.head);
	} else if (!_graph->has_arc(tail, head)) {
		return Event::pre_process_done(Status::NOT_FOUND, _msg.head);
	}

	if (tail_block == nullptr || head_block == nullptr) {
		return Event::pre_process_done(Status::PARENT_NOT_FOUND, _msg.head);
	}

	_impl = make_unique<Impl>(_engine,
	                          _graph,
	                          dynamic_cast<PortImpl*>(tail),
	                          dynamic_cast<InputPort*>(head));

	_compiled_graph = ctx.maybe_compile(*_engine.maid(), *_graph);

	return Event::pre_process_done(Status::SUCCESS);
}

bool
Disconnect::Impl::execute(RunContext& context, bool set_head_buffers)
{
	if (!_arc) {
		return false;
	}

	_head->remove_arc(*_arc.get());
	if (_head->is_driver_port()) {
		return true;
	}

	if (set_head_buffers) {
		if (_voices) {
			_head->set_voices(context, std::move(_voices));
		} else {
			_head->setup_buffers(context, *_engine.buffer_factory(), _head->poly());
		}
		_head->connect_buffers();
	} else {
		_head->recycle_buffers();
	}

	return true;
}

void
Disconnect::execute(RunContext& context)
{
	if (_status == Status::SUCCESS) {
		if (_impl->execute(context, true)) {
			if (_compiled_graph) {
				_graph->set_compiled_graph(std::move(_compiled_graph));
			}
		} else {
			_status = Status::NOT_FOUND;
		}
	}
}

void
Disconnect::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS) {
		_engine.broadcaster()->message(_msg);
	}
}

void
Disconnect::undo(Interface& target)
{
	target.connect(_msg.tail, _msg.head);
}

} // namespace events
} // namespace server
} // namespace ingen
