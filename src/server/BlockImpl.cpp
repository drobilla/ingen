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

#include <assert.h>
#include <stdint.h>

#include "raul/Array.hpp"

#include "Buffer.hpp"
#include "Engine.hpp"
#include "BlockImpl.hpp"
#include "GraphImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"
#include "RunContext.hpp"
#include "ThreadManager.hpp"

using namespace std;

namespace Ingen {
namespace Server {

BlockImpl::BlockImpl(PluginImpl*         plugin,
                     const Raul::Symbol& symbol,
                     bool                polyphonic,
                     GraphImpl*          parent,
                     SampleRate          srate)
	: NodeImpl(plugin->uris(), parent, symbol)
	, _plugin(plugin)
	, _ports(NULL)
	, _polyphony((polyphonic && parent) ? parent->internal_poly() : 1)
	, _polyphonic(polyphonic)
	, _activated(false)
	, _enabled(true)
	, _traversed(false)
{
	assert(_plugin);
	assert(_polyphony > 0);
}

BlockImpl::~BlockImpl()
{
	if (_activated) {
		deactivate();
	}

	if (is_linked()) {
		parent_graph()->remove_block(*this);
	}

	delete _ports;
}

Node*
BlockImpl::port(uint32_t index) const
{
	return (*_ports)[index];
}

const Resource*
BlockImpl::plugin() const
{
	return _plugin;
}

const PluginImpl*
BlockImpl::plugin_impl() const
{
	return _plugin;
}

void
BlockImpl::activate(BufferFactory& bufs)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	_activated = true;
	for (uint32_t p = 0; p < num_ports(); ++p) {
		PortImpl* const port = _ports->at(p);
		port->activate(bufs);
	}
}

void
BlockImpl::deactivate()
{
	_activated = false;
	for (uint32_t p = 0; p < num_ports(); ++p) {
		PortImpl* const port = _ports->at(p);
		port->deactivate();
	}
}

bool
BlockImpl::prepare_poly(BufferFactory& bufs, uint32_t poly)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	if (!_polyphonic)
		poly = 1;

	if (_ports)
		for (uint32_t i = 0; i < _ports->size(); ++i)
			_ports->at(i)->prepare_poly(bufs, poly);

	return true;
}

bool
BlockImpl::apply_poly(RunContext& context, Raul::Maid& maid, uint32_t poly)
{
	if (!_polyphonic)
		poly = 1;

	_polyphony = poly;

	if (_ports)
		for (uint32_t i = 0; i < num_ports(); ++i)
			_ports->at(i)->apply_poly(context, maid, poly);

	return true;
}

void
BlockImpl::set_buffer_size(RunContext&    context,
                           BufferFactory& bufs,
                           LV2_URID       type,
                           uint32_t       size)
{
	if (_ports) {
		for (uint32_t i = 0; i < _ports->size(); ++i) {
			PortImpl* const p = _ports->at(i);
			if (p->buffer_type() == type) {
				p->set_buffer_size(context, bufs, size);
			}
		}
	}
}

PortImpl*
BlockImpl::nth_port_by_type(uint32_t n, bool input, PortType type)
{
	uint32_t count = 0;
	for (uint32_t i = 0; _ports && i < _ports->size(); ++i) {
		PortImpl* const port = _ports->at(i);
		if (port->is_input() == input && port->type() == type) {
			if (count++ == n) {
				return port;
			}
		}
	}
	return NULL;
}

PortImpl*
BlockImpl::port_by_symbol(const char* symbol)
{
	for (uint32_t p = 0; _ports && p < _ports->size(); ++p) {
		if (_ports->at(p)->symbol() == symbol) {
			return _ports->at(p);
		}
	}
	return NULL;
}

void
BlockImpl::pre_process(RunContext& context)
{
	// Mix down input ports
	for (uint32_t i = 0; i < num_ports(); ++i) {
		PortImpl* const port = _ports->at(i);
		port->pre_process(context);
		port->connect_buffers();
	}
}

void
BlockImpl::process(RunContext& context)
{
	pre_process(context);

	if (!_enabled) {
		// Prepare port buffers for reading, converting/mixing if necessary
		for (uint32_t i = 0; _ports && i < _ports->size(); ++i) {
			_ports->at(i)->connect_buffers();
			_ports->at(i)->pre_run(context);
		}

		// Dumb bypass
		for (PortType t : { PortType::AUDIO, PortType::CV, PortType::ATOM }) {
			for (uint32_t i = 0;; ++i) {
				PortImpl* in  = nth_port_by_type(i, true, t);
				PortImpl* out = nth_port_by_type(i, false, t);
				if (!out) {
					break;  // Finished writing all outputs
				} else if (in) {
					// Copy corresponding input to output
					for (uint32_t v = 0; v < _polyphony; ++v) {
						out->buffer(v)->copy(context, in->buffer(v).get());
					}
				} else {
					// Output but no corresponding input, clear
					for (uint32_t v = 0; v < _polyphony; ++v) {
						out->buffer(v)->clear();
					}
				}
			}
		}
		post_process(context);
		return;
	}

	RunContext subcontext(context);
	for (SampleCount offset = 0; offset < context.nframes();) {
		// Find earliest offset of a value change
		SampleCount chunk_end = context.nframes();
		for (uint32_t i = 0; _ports && i < _ports->size(); ++i) {
			PortImpl* const port = _ports->at(i);
			if (port->type() == PortType::CONTROL && port->is_input()) {
				const SampleCount o = port->next_value_offset(
					offset, context.nframes());
				if (o < chunk_end) {
					chunk_end = o;
				}
			}
		}

		// Slice context into a chunk from now until the next change
		subcontext.slice(offset, chunk_end - offset);

		// Prepare port buffers for reading, converting/mixing if necessary
		for (uint32_t i = 0; _ports && i < _ports->size(); ++i) {
			_ports->at(i)->connect_buffers(offset);
			_ports->at(i)->pre_run(subcontext);
		}

		// Run the chunk
		run(subcontext);

		offset = chunk_end;
		subcontext.slice(offset, chunk_end - offset);
	}

	post_process(context);
}

void
BlockImpl::post_process(RunContext& context)
{
	// Write output ports
	for (uint32_t i = 0; _ports && i < _ports->size(); ++i) {
		_ports->at(i)->post_process(context);
	}
}

void
BlockImpl::set_port_buffer(uint32_t    voice,
                           uint32_t    port_num,
                           BufferRef   buf,
                           SampleCount offset)
{
	/*std::cout << path() << " set port " << port_num << " voice " << voice
	  << " buffer " << buf << " offset " << offset << std::endl;*/
}

} // namespace Server
} // namespace Ingen
