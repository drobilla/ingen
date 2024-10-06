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

#include "DuplexPort.hpp"

#include "Buffer.hpp"
#include "BufferFactory.hpp"
#include "BufferRef.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "NodeImpl.hpp"
#include "PortType.hpp"

#include "ingen/Atom.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Node.hpp"
#include "ingen/Properties.hpp"
#include "ingen/URIs.hpp"
#include "lv2/urid/urid.h"
#include "raul/Array.hpp"
#include "raul/Maid.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <utility>

namespace ingen::server {

DuplexPort::DuplexPort(BufferFactory&      bufs,
                       GraphImpl*          parent,
                       const raul::Symbol& symbol,
                       uint32_t            index,
                       bool                polyphonic,
                       PortType            type,
                       LV2_URID            buf_type,
                       size_t              buf_size,
                       const Atom&         value,
                       bool                is_output)
	: InputPort(bufs, parent, symbol, index, parent->polyphony(), type, buf_type, value, buf_size)
{
	if (polyphonic) {
		set_property(bufs.uris().ingen_polyphonic, bufs.forge().make(true));
	}

	if (!parent->parent() ||
	    _poly != parent->parent_graph()->internal_poly()) {
		_poly = 1;
	}

	// Set default control range
	if (!is_output && value.type() == bufs.uris().atom_Float) {
		set_property(bufs.uris().lv2_minimum, bufs.forge().make(0.0f));
		set_property(bufs.uris().lv2_maximum, bufs.forge().make(1.0f));
	}

	_is_output = is_output;
	if (is_output) {
		if (parent->graph_type() != Node::GraphType::GRAPH) {
			remove_property(bufs.uris().rdf_type,
			                bufs.uris().lv2_InputPort.urid_atom());

			add_property(bufs.uris().rdf_type,
			             bufs.uris().lv2_OutputPort.urid_atom());
		}
	}

	DuplexPort::get_buffers(bufs, &BufferFactory::get_buffer,
	                        _voices, parent->polyphony(), 0);
}

DuplexPort::~DuplexPort()
{
	if (is_linked()) {
		parent_graph()->remove_port(*this);
	}
}

DuplexPort*
DuplexPort::duplicate(Engine&             engine,
                      const raul::Symbol& symbol,
                      GraphImpl*          parent)
{
	BufferFactory& bufs       = *engine.buffer_factory();
	const Atom     polyphonic = get_property(bufs.uris().ingen_polyphonic);

	auto* dup = new DuplexPort(
		bufs, parent, symbol, _index,
		polyphonic.type() == bufs.uris().atom_Bool && polyphonic.get<int32_t>(),
		_type, _buffer_type, _buffer_size,
		_value, _is_output);

	dup->set_properties(properties());

	return dup;
}

void
DuplexPort::inherit_neighbour(const PortImpl* port,
                              Properties&     remove,
                              Properties&     add)
{
	const URIs& uris = _bufs.uris();

	/* TODO: This needs to become more sophisticated, and correct the situation
	   if the port is disconnected. */
	if (_type == PortType::CONTROL || _type == PortType::CV) {
		if (port->minimum().get<float>() < _min.get<float>()) {
			_min = port->minimum();
			remove.emplace(uris.lv2_minimum, uris.patch_wildcard);
			add.emplace(uris.lv2_minimum, port->minimum());
		}
		if (port->maximum().get<float>() > _max.get<float>()) {
			_max = port->maximum();
			remove.emplace(uris.lv2_maximum, uris.patch_wildcard);
			add.emplace(uris.lv2_maximum, port->maximum());
		}
	} else if (_type == PortType::ATOM) {
		for (auto i = port->properties().find(uris.atom_supports);
		     i != port->properties().end() && i->first == uris.atom_supports;
		     ++i) {
			set_property(i->first, i->second);
			add.insert(*i);
		}
	}
}

void
DuplexPort::on_property(const URI& uri, const Atom& value)
{
	_bufs.engine().driver()->port_property(_path, uri, value);
}

bool
DuplexPort::get_buffers(BufferFactory&                   bufs,
                        PortImpl::GetFn                  get,
                        const raul::managed_ptr<Voices>& voices,
                        uint32_t                         poly,
                        size_t                           num_in_arcs) const
{
	if (!_is_driver_port && is_output()) {
		return InputPort::get_buffers(bufs, get, voices, poly, num_in_arcs);
	}

	if (!_is_driver_port && is_input()) {
		return PortImpl::get_buffers(bufs, get, voices, poly, num_in_arcs);
	}

	return false;
}

bool
DuplexPort::setup_buffers(RunContext& ctx, BufferFactory& bufs, uint32_t poly)
{
	if (!_is_driver_port && is_output()) {
		return InputPort::setup_buffers(ctx, bufs, poly);
	}

	if (!_is_driver_port && is_input()) {
		return PortImpl::setup_buffers(ctx, bufs, poly);
	}

	return false;
}

void
DuplexPort::set_is_driver_port(BufferFactory& bufs)
{
	_voices->at(0).buffer = new Buffer(bufs, buffer_type(), _value.type(), 0, true, nullptr);
	PortImpl::set_is_driver_port(bufs);
}

void
DuplexPort::set_driver_buffer(void* buf, uint32_t capacity)
{
	_voices->at(0).buffer->set_buffer(buf);
	_voices->at(0).buffer->set_capacity(capacity);
}

uint32_t
DuplexPort::max_tail_poly(RunContext&) const
{
	return std::max(_poly, parent_graph()->internal_poly_process());
}

bool
DuplexPort::prepare_poly(BufferFactory& bufs, uint32_t poly)
{
	if (!parent()->parent() ||
	    poly != parent()->parent_graph()->internal_poly()) {
		return false;
	}

	return PortImpl::prepare_poly(bufs, poly);
}

bool
DuplexPort::apply_poly(RunContext& ctx, uint32_t poly)
{
	if (!parent()->parent() ||
	    poly != parent()->parent_graph()->internal_poly()) {
		return false;
	}

	return PortImpl::apply_poly(ctx, poly);
}

void
DuplexPort::pre_process(RunContext& ctx)
{
	if (_is_output) {
		/* This is a graph output, which is an input from the internal
		   perspective.  Prepare buffers for write so plugins can deliver to
		   them */
		for (uint32_t v = 0; v < _poly; ++v) {
			_voices->at(v).buffer->prepare_write(ctx);
		}
	} else {
		/* This is a a graph input, which is an output from the internal
		   perspective.  Do whatever a normal block's input port does to
		   prepare input for reading. */
		InputPort::pre_process(ctx);
		InputPort::pre_run(ctx);
	}
}

void
DuplexPort::post_process(RunContext& ctx)
{
	if (_is_output) {
		/* This is a graph output, which is an input from the internal
		   perspective.  Mix down input delivered by plugins so output
		   (external perspective) is ready. */
		InputPort::pre_process(ctx);
		InputPort::pre_run(ctx);
	}
	monitor(ctx);
}

SampleCount
DuplexPort::next_value_offset(SampleCount offset, SampleCount end) const
{
	return PortImpl::next_value_offset(offset, end);
}

} // namespace ingen::server
