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

#include "ingen/URIs.hpp"

#include "Buffer.hpp"
#include "Driver.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "OutputPort.hpp"

using namespace std;

namespace Ingen {
namespace Server {

DuplexPort::DuplexPort(BufferFactory&      bufs,
                       BlockImpl*          parent,
                       const Raul::Symbol& symbol,
                       uint32_t            index,
                       bool                polyphonic,
                       uint32_t            poly,
                       PortType            type,
                       LV2_URID            buffer_type,
                       const Atom&         value,
                       size_t              buffer_size,
                       bool                is_output)
	: PortImpl(bufs, parent, symbol, index, poly, type, buffer_type, value, buffer_size)
	, InputPort(bufs, parent, symbol, index, poly, type, buffer_type, value, buffer_size)
	, OutputPort(bufs, parent, symbol, index, poly, type, buffer_type, value, buffer_size)
	, _is_output(is_output)
{
	if (polyphonic) {
		set_property(bufs.uris().ingen_polyphonic, bufs.forge().make(true));
	}

	// Set default control range
	if (!is_output && value.type() == bufs.uris().atom_Float) {
		set_property(bufs.uris().lv2_minimum, bufs.forge().make(0.0f));
		set_property(bufs.uris().lv2_maximum, bufs.forge().make(1.0f));
	}
}

DuplexPort::~DuplexPort()
{
	if (is_linked()) {
		parent_graph()->remove_port(*this);
	}
}

void
DuplexPort::inherit_neighbour(const PortImpl*       port,
                              Resource::Properties& remove,
                              Resource::Properties& add)
{
	/* TODO: This needs to become more sophisticated, and correct the situation
	   if the port is disconnected. */
	if (_type == PortType::CONTROL || _type == PortType::CV) {
		if (port->minimum().get<float>() < _min.get<float>()) {
			_min = port->minimum();
			remove.insert(std::make_pair(_bufs.uris().lv2_minimum,
			                             Property(_bufs.uris().patch_wildcard)));
			add.insert(std::make_pair(_bufs.uris().lv2_minimum,
			                          port->minimum()));
		}
		if (port->maximum().get<float>() > _max.get<float>()) {
			_max = port->maximum();
			remove.insert(std::make_pair(_bufs.uris().lv2_maximum,
			                             Property(_bufs.uris().patch_wildcard)));
			add.insert(std::make_pair(_bufs.uris().lv2_maximum,
			                          port->maximum()));
		}
	} else if (_type == PortType::ATOM) {
		for (Resource::Properties::const_iterator i = port->properties().find(
			     _bufs.uris().atom_supports);
		     i != port->properties().end() &&
			     i->first == _bufs.uris().atom_supports;
		     ++i) {
			set_property(i->first, i->second);
			add.insert(*i);
		}
	}
}

void
DuplexPort::on_property(const Raul::URI& uri, const Atom& value)
{
	_bufs.engine().driver()->port_property(_path, uri, value);
}

bool
DuplexPort::get_buffers(BufferFactory&      bufs,
                        Raul::Array<Voice>* voices,
                        uint32_t            poly,
                        bool                real_time) const
{
	if (_is_output) {
		return InputPort::get_buffers(bufs, voices, poly, real_time);
	} else {
		return OutputPort::get_buffers(bufs, voices, poly, real_time);
	}
}

uint32_t
DuplexPort::max_tail_poly(Context& context) const
{
	return parent_graph()->internal_poly_process();
}

void
DuplexPort::pre_process(Context& context)
{
	if (_is_output) {
		/* This is a graph output, which is an input from the internal
		   perspective.  Prepare buffers for write so plugins can deliver to
		   them */
		for (uint32_t v = 0; v < _poly; ++v) {
			_voices->at(v).buffer->prepare_write(context);
		}
	} else {
		/* This is a a graph input, which is an output from the internal
		   perspective.  Do whatever a normal block's input port does to
		   prepare input for reading. */
		InputPort::pre_process(context);
	}
}

void
DuplexPort::post_process(Context& context)
{
	if (_is_output) {
		/* This is a graph output, which is an input from the internal
		   perspective.  Mix down input delivered by plugins so output
		   (external perspective) is ready. */
		InputPort::pre_process(context);
		InputPort::pre_run(context);
	} else {
		monitor(context);
	}
}

SampleCount
DuplexPort::next_value_offset(SampleCount offset, SampleCount end) const
{
	return OutputPort::next_value_offset(offset, end);
}

void
DuplexPort::update_values(SampleCount offset)
{
	return OutputPort::update_values(offset);
}

} // namespace Server
} // namespace Ingen
