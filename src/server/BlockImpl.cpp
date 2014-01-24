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

#include <assert.h>
#include <stdint.h>

#include "raul/Array.hpp"

#include "Buffer.hpp"
#include "Engine.hpp"
#include "BlockImpl.hpp"
#include "GraphImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"
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
	, _context(Context::ID::AUDIO)
	, _polyphony((polyphonic && parent) ? parent->internal_poly() : 1)
	, _polyphonic(polyphonic)
	, _activated(false)
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

const Plugin*
BlockImpl::plugin() const
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
BlockImpl::apply_poly(ProcessContext& context, Raul::Maid& maid, uint32_t poly)
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
BlockImpl::set_buffer_size(Context&       context,
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

/** Prepare to run a cycle (in the audio thread)
 */
void
BlockImpl::pre_process(ProcessContext& context)
{
	// Mix down input ports
	for (uint32_t i = 0; i < num_ports(); ++i) {
		PortImpl* const port = _ports->at(i);
		port->pre_process(context);
		port->connect_buffers();
	}
}

/** Prepare to run a cycle (in the audio thread)
 */
void
BlockImpl::post_process(ProcessContext& context)
{
	// Write output ports
	for (uint32_t i = 0; _ports && i < _ports->size(); ++i) {
		_ports->at(i)->post_process(context);
	}
}

void
BlockImpl::set_port_buffer(uint32_t  voice,
                           uint32_t  port_num,
                           BufferRef buf)
{
	/*std::cout << path() << " set port " << port_num << " voice " << voice
	  << " buffer " << buf << " offset " << offset << std::endl;*/
}

} // namespace Server
} // namespace Ingen
