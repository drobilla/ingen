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

#include "GraphImpl.hpp"

#include "ArcImpl.hpp"
#include "BlockImpl.hpp"
#include "BufferFactory.hpp"
#include "CompiledGraph.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "GraphPlugin.hpp"
#include "PortImpl.hpp"
#include "ThreadManager.hpp"

#include "ingen/Forge.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "raul/Maid.hpp"

#include <cassert>
#include <cstddef>
#include <map>
#include <unordered_map>

namespace ingen {
namespace server {

GraphImpl::GraphImpl(Engine&             engine,
                     const Raul::Symbol& symbol,
                     uint32_t            poly,
                     GraphImpl*          parent,
                     SampleRate          srate,
                     uint32_t            internal_poly)
	: BlockImpl(new GraphPlugin(engine.world().uris(),
	                            engine.world().uris().ingen_Graph,
	                            Raul::Symbol("graph"),
	                            "Ingen Graph"),
	            symbol, poly, parent, srate)
	, _engine(engine)
	, _poly_pre(internal_poly)
	, _poly_process(internal_poly)
	, _process(false)
{
	assert(internal_poly >= 1);
	assert(internal_poly <= 128);
}

GraphImpl::~GraphImpl()
{
	delete _plugin;
}

BlockImpl*
GraphImpl::duplicate(Engine&             engine,
                     const Raul::Symbol& symbol,
                     GraphImpl*          parent)
{
	BufferFactory&   bufs = *engine.buffer_factory();
	const SampleRate rate = engine.sample_rate();

	// Duplicate graph
	GraphImpl* dup = new GraphImpl(
		engine, symbol, _polyphony, parent, rate, _poly_process);

	Properties props = properties();
	props.erase(bufs.uris().lv2_symbol);
	props.insert({bufs.uris().lv2_symbol, bufs.forge().alloc(symbol.c_str())});
	dup->set_properties(props);

	// We need a map of port duplicates to duplicate arcs
	using PortMap = std::unordered_map<PortImpl*, PortImpl*>;
	PortMap port_map;

	// Add duplicates of all ports
	dup->_ports = bufs.maid().make_managed<Ports>(num_ports(), nullptr);
	for (PortList::iterator p = _inputs.begin(); p != _inputs.end(); ++p) {
		DuplexPort* p_dup = p->duplicate(engine, p->symbol(), dup);
		dup->_inputs.push_front(*p_dup);
		(*dup->_ports)[p->index()] = p_dup;
		port_map.insert({&*p, p_dup});
	}
	for (PortList::iterator p = _outputs.begin(); p != _outputs.end(); ++p) {
		DuplexPort* p_dup = p->duplicate(engine, p->symbol(), dup);
		dup->_outputs.push_front(*p_dup);
		(*dup->_ports)[p->index()] = p_dup;
		port_map.insert({&*p, p_dup});
	}

	// Add duplicates of all blocks
	for (auto& b : _blocks) {
		BlockImpl* b_dup = b.duplicate(engine, b.symbol(), dup);
		dup->add_block(*b_dup);
		b_dup->activate(*engine.buffer_factory());
		for (uint32_t p = 0; p < b.num_ports(); ++p) {
			port_map.insert({b.port_impl(p), b_dup->port_impl(p)});
		}
	}

	// Add duplicates of all arcs
	for (const auto& a : _arcs) {
		SPtr<ArcImpl> arc = dynamic_ptr_cast<ArcImpl>(a.second);
		if (arc) {
			auto t = port_map.find(arc->tail());
			auto h = port_map.find(arc->head());
			if (t != port_map.end() && h != port_map.end()) {
				dup->add_arc(std::make_shared<ArcImpl>(t->second, h->second));
			}
		}
	}

	return dup;
}

void
GraphImpl::activate(BufferFactory& bufs)
{
	BlockImpl::activate(bufs);

	for (auto& b : _blocks) {
		b.activate(bufs);
	}

	assert(_activated);
}

void
GraphImpl::deactivate()
{
	if (_activated) {
		BlockImpl::deactivate();

		for (auto& b : _blocks) {
			if (b.activated()) {
				b.deactivate();
			}
		}
	}
}

void
GraphImpl::disable(RunContext& context)
{
	_process = false;
	for (auto& o : _outputs) {
		o.clear_buffers(context);
	}
}

bool
GraphImpl::prepare_internal_poly(BufferFactory& bufs, uint32_t poly)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	// TODO: Subgraph dynamic polyphony (i.e. changing port polyphony)

	for (auto& b : _blocks) {
		b.prepare_poly(bufs, poly);
	}

	_poly_pre = poly;
	return true;
}

bool
GraphImpl::apply_internal_poly(RunContext&    context,
                               BufferFactory& bufs,
                               Raul::Maid&    maid,
                               uint32_t       poly)
{
	// TODO: Subgraph dynamic polyphony (i.e. changing port polyphony)

	for (auto& b : _blocks) {
		b.apply_poly(context, poly);
	}

	for (auto& b : _blocks) {
		for (uint32_t j = 0; j < b.num_ports(); ++j) {
			PortImpl* const port = b.port_impl(j);
			if (port->is_input() && dynamic_cast<InputPort*>(port)->direct_connect()) {
				port->setup_buffers(context, bufs, port->poly());
			}
			port->connect_buffers();
		}
	}

	const bool polyphonic = parent_graph() && (poly == parent_graph()->internal_poly_process());
	for (auto& o : _outputs) {
		o.setup_buffers(context, bufs, polyphonic ? poly : 1);
	}

	_poly_process = poly;
	return true;
}

void
GraphImpl::pre_process(RunContext& context)
{
	// Mix down input ports and connect buffers
	for (uint32_t i = 0; i < num_ports(); ++i) {
		PortImpl* const port = _ports->at(i);
		if (!port->is_driver_port()) {
			port->pre_process(context);
			port->pre_run(context);
			port->connect_buffers();
		}
	}
}

void
GraphImpl::process(RunContext& context)
{
	if (!_process) {
		return;
	}

	pre_process(context);
	run(context);
	post_process(context);
}

void
GraphImpl::run(RunContext& context)
{
	if (_compiled_graph) {
		_compiled_graph->run(context);
	}
}

void
GraphImpl::set_buffer_size(RunContext&    context,
                           BufferFactory& bufs,
                           LV2_URID       type,
                           uint32_t       size)
{
	BlockImpl::set_buffer_size(context, bufs, type, size);

	if (_compiled_graph) {
		// FIXME
		// for (size_t i = 0; i < _compiled_graph->size(); ++i) {
		// 	const CompiledBlock& block = (*_compiled_graph)[i];
		// 	block.block()->set_buffer_size(context, bufs, type, size);
		// }
	}
}

void
GraphImpl::add_block(BlockImpl& block)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	_blocks.push_front(block);
}

void
GraphImpl::remove_block(BlockImpl& block)
{
	_blocks.erase(_blocks.iterator_to(block));
}

void
GraphImpl::add_arc(SPtr<ArcImpl> a)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	_arcs.emplace(std::make_pair(a->tail(), a->head()), a);
}

SPtr<ArcImpl>
GraphImpl::remove_arc(const PortImpl* tail, const PortImpl* dst_port)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	auto i = _arcs.find(std::make_pair(tail, dst_port));
	if (i != _arcs.end()) {
		SPtr<ArcImpl> arc = dynamic_ptr_cast<ArcImpl>(i->second);
		_arcs.erase(i);
		return arc;
	} else {
		return SPtr<ArcImpl>();
	}
}

bool
GraphImpl::has_arc(const PortImpl* tail, const PortImpl* dst_port) const
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	auto i = _arcs.find(std::make_pair(tail, dst_port));
	return (i != _arcs.end());
}

void
GraphImpl::set_compiled_graph(MPtr<CompiledGraph>&& cg)
{
	if (_compiled_graph && _compiled_graph != cg) {
		_engine.reset_load();
	}
	_compiled_graph = std::move(cg);
}

uint32_t
GraphImpl::num_ports_non_rt() const
{
	ThreadManager::assert_not_thread(THREAD_PROCESS);
	return _inputs.size() + _outputs.size();
}

bool
GraphImpl::has_port_with_index(uint32_t index) const
{
	BufferFactory& bufs       = *_engine.buffer_factory();
	const auto     index_atom = bufs.forge().make(int32_t(index));

	for (auto p = _inputs.begin(); p != _inputs.end(); ++p) {
		if (p->has_property(bufs.uris().lv2_index, index_atom)) {
			return true;
		}
	}

	for (auto p = _outputs.begin(); p != _outputs.end(); ++p) {
		if (p->has_property(bufs.uris().lv2_index, index_atom)) {
			return true;
		}
	}

	return false;
}

void
GraphImpl::remove_port(DuplexPort& port)
{
	if (port.is_input()) {
		_inputs.erase(_inputs.iterator_to(port));
	} else {
		_outputs.erase(_outputs.iterator_to(port));
	}
}

void
GraphImpl::clear_ports()
{
	_inputs.clear();
	_outputs.clear();
}

MPtr<BlockImpl::Ports>
GraphImpl::build_ports_array(Raul::Maid& maid)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	const size_t n = _inputs.size() + _outputs.size();
	MPtr<Ports> result = maid.make_managed<Ports>(n);

	std::map<size_t, DuplexPort*> ports;
	for (PortList::iterator p = _inputs.begin(); p != _inputs.end(); ++p) {
		ports.emplace(p->index(), &*p);
	}
	for (PortList::iterator p = _outputs.begin(); p != _outputs.end(); ++p) {
		ports.emplace(p->index(), &*p);
	}

	size_t i = 0;
	for (const auto& p : ports) {
		result->at(i++) = p.second;
	}

	assert(i == n);

	return result;
}

} // namespace server
} // namespace ingen
