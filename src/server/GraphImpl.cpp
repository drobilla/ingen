/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>
#include <unordered_map>

#include "ingen/Log.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "raul/Maid.hpp"

#include "ArcImpl.hpp"
#include "BlockImpl.hpp"
#include "BufferFactory.hpp"
#include "Driver.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "GraphPlugin.hpp"
#include "PortImpl.hpp"
#include "ThreadManager.hpp"

using namespace std;

namespace Ingen {
namespace Server {

GraphImpl::GraphImpl(Engine&             engine,
                     const Raul::Symbol& symbol,
                     uint32_t            poly,
                     GraphImpl*          parent,
                     SampleRate          srate,
                     uint32_t            internal_poly)
	: BlockImpl(new GraphPlugin(engine.world()->uris(),
	                            engine.world()->uris().ingen_Graph,
	                            Raul::Symbol("graph"),
	                            "Ingen Graph"),
	            symbol, poly, parent, srate)
	, _engine(engine)
	, _poly_pre(internal_poly)
	, _poly_process(internal_poly)
	, _compiled_graph(NULL)
	, _process(false)
{
	assert(internal_poly >= 1);
	assert(internal_poly <= 128);
}

GraphImpl::~GraphImpl()
{
	delete _compiled_graph;
	delete _plugin;
}

BlockImpl*
GraphImpl::duplicate(Engine&             engine,
                     const Raul::Symbol& symbol,
                     GraphImpl*          parent)
{
	BufferFactory&   bufs = *engine.buffer_factory();
	const SampleRate rate = engine.driver()->sample_rate();

	// Duplicate graph
	GraphImpl* dup = new GraphImpl(
		engine, symbol, _polyphony, parent, rate, _poly_process);

	Properties props = properties();
	props.erase(bufs.uris().lv2_symbol);
	props.insert({bufs.uris().lv2_symbol, bufs.forge().alloc(symbol.c_str())});
	dup->set_properties(props);

	// We need a map of port duplicates to duplicate arcs
	typedef std::unordered_map<PortImpl*, PortImpl*> PortMap;
	PortMap port_map;

	// Add duplicates of all ports
	dup->_ports = new Raul::Array<PortImpl*>(num_ports(), NULL);
	for (Ports::iterator p = _inputs.begin(); p != _inputs.end(); ++p) {
		DuplexPort* p_dup = p->duplicate(engine, p->symbol(), dup);
		dup->_inputs.push_front(*p_dup);
		(*dup->_ports)[p->index()] = p_dup;
		port_map.insert({&*p, p_dup});
	}
	for (Ports::iterator p = _outputs.begin(); p != _outputs.end(); ++p) {
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
			PortMap::iterator t = port_map.find(arc->tail());
			PortMap::iterator h = port_map.find(arc->head());
			if (t != port_map.end() && h != port_map.end()) {
				dup->add_arc(SPtr<ArcImpl>(new ArcImpl(t->second, h->second)));
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
GraphImpl::disable(ProcessContext& context)
{
	_process = false;
	for (auto& o : _outputs) {
		o.clear_buffers();
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
GraphImpl::apply_internal_poly(ProcessContext& context,
                               BufferFactory&  bufs,
                               Raul::Maid&     maid,
                               uint32_t        poly)
{
	// TODO: Subgraph dynamic polyphony (i.e. changing port polyphony)

	for (auto& b : _blocks) {
		b.apply_poly(context, maid, poly);
	}

	for (auto& b : _blocks) {
		for (uint32_t j = 0; j < b.num_ports(); ++j) {
			PortImpl* const port = b.port_impl(j);
			if (port->is_input() && dynamic_cast<InputPort*>(port)->direct_connect())
				port->setup_buffers(bufs, port->poly(), true);
			port->connect_buffers();
		}
	}

	const bool polyphonic = parent_graph() && (poly == parent_graph()->internal_poly_process());
	for (auto& o : _outputs)
		o.setup_buffers(bufs, polyphonic ? poly : 1, true);

	_poly_process = poly;
	return true;
}

void
GraphImpl::pre_process(ProcessContext& context)
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
GraphImpl::process(ProcessContext& context)
{
	if (!_process)
		return;

	pre_process(context);
	run(context);
	post_process(context);
}

void
GraphImpl::run(ProcessContext& context)
{
	if (_compiled_graph && _compiled_graph->size() > 0) {
		// Run all blocks
		for (size_t i = 0; i < _compiled_graph->size(); ++i) {
			(*_compiled_graph)[i].block()->process(context);
		}
	}
}

void
GraphImpl::set_buffer_size(Context&       context,
                           BufferFactory& bufs,
                           LV2_URID       type,
                           uint32_t       size)
{
	BlockImpl::set_buffer_size(context, bufs, type, size);

	for (size_t i = 0; i < _compiled_graph->size(); ++i)
		(*_compiled_graph)[i].block()->set_buffer_size(context, bufs, type, size);
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
	_arcs.insert(make_pair(make_pair(a->tail(), a->head()), a));
}

SPtr<ArcImpl>
GraphImpl::remove_arc(const PortImpl* tail, const PortImpl* dst_port)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	Arcs::iterator i = _arcs.find(make_pair(tail, dst_port));
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
	Arcs::const_iterator i = _arcs.find(make_pair(tail, dst_port));
	return (i != _arcs.end());
}

void
GraphImpl::set_compiled_graph(CompiledGraph* cg)
{
	if (_compiled_graph && _compiled_graph != cg) {
		_engine.maid()->dispose(_compiled_graph);
	}
	_compiled_graph = cg;
}

uint32_t
GraphImpl::num_ports_non_rt() const
{
	ThreadManager::assert_not_thread(THREAD_PROCESS);
	return _inputs.size() + _outputs.size();
}

DuplexPort*
GraphImpl::create_port(BufferFactory&      bufs,
                       const Raul::Symbol& symbol,
                       PortType            type,
                       LV2_URID            buffer_type,
                       uint32_t            buffer_size,
                       bool                is_output,
                       bool                polyphonic)
{
	if (type == PortType::UNKNOWN) {
		bufs.engine().log().error(fmt("Unknown port type %1%\n")
		                          % type.uri());
		return NULL;
	}

	Atom value;
	if (type == PortType::CONTROL || type == PortType::CV)
		value = bufs.forge().make(0.0f);

	return new DuplexPort(bufs, this, symbol, num_ports_non_rt(), polyphonic, _polyphony,
	                      type, buffer_type, value, buffer_size, is_output);
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

Raul::Array<PortImpl*>*
GraphImpl::build_ports_array()
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	const size_t n = _inputs.size() + _outputs.size();
	Raul::Array<PortImpl*>* const result = new Raul::Array<PortImpl*>(n);

	size_t i = 0;

	for (Ports::iterator p = _inputs.begin(); p != _inputs.end(); ++p, ++i)
		result->at(i) = &*p;

	for (Ports::iterator p = _outputs.begin(); p != _outputs.end(); ++p, ++i)
		result->at(i) = &*p;

	assert(i == n);

	return result;
}

static inline void
compile_recursive(BlockImpl* n, CompiledGraph* output)
{
	if (n == NULL || n->traversed())
		return;

	n->traversed(true);
	assert(output != NULL);

	for (auto& p : n->providers())
		if (!p->traversed())
			compile_recursive(p, output);

	output->push_back(CompiledBlock(n, n->providers().size(), n->dependants()));
}

CompiledGraph*
GraphImpl::compile()
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	CompiledGraph* const compiled_graph = new CompiledGraph();

	for (auto& b : _blocks) {
		b.traversed(false);
	}

	for (auto& b : _blocks) {
		// Either a sink or connected to our output ports:
		if (!b.traversed() && b.dependants().empty()) {
			compile_recursive(&b, compiled_graph);
		}
	}

	// Traverse any blocks we didn't hit yet
	for (auto& b : _blocks) {
		if (!b.traversed()) {
			compile_recursive(&b, compiled_graph);
		}
	}

	if (compiled_graph->size() != _blocks.size()) {
		_engine.log().error(fmt("Failed to compile graph %1%\n") % _path);
		delete compiled_graph;
		return NULL;
	}

	return compiled_graph;
}

} // namespace Server
} // namespace Ingen
