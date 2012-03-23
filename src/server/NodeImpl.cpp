/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <assert.h>
#include <stdint.h>

#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "raul/Array.hpp"
#include "raul/List.hpp"

#include "AudioBuffer.hpp"
#include "ClientBroadcaster.hpp"
#include "EngineStore.hpp"
#include "NodeImpl.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"
#include "ThreadManager.hpp"
#include "util.hpp"

using namespace std;

namespace Ingen {
namespace Server {

NodeImpl::NodeImpl(PluginImpl*         plugin,
                   const Raul::Symbol& symbol,
                   bool                polyphonic,
                   PatchImpl*          parent,
                   SampleRate          srate)
	: GraphObjectImpl(plugin->uris(), parent, symbol)
	, _plugin(plugin)
	, _ports(NULL)
	, _context(Context::AUDIO)
	, _polyphony((polyphonic && parent) ? parent->internal_poly() : 1)
	, _srate(srate)
	, _input_ready(1)
	, _process_lock(0)
	, _n_inputs_ready(0)
	, _polyphonic(polyphonic)
	, _activated(false)
	, _traversed(false)
{
	assert(_plugin);
	assert(_polyphony > 0);
}

NodeImpl::~NodeImpl()
{
	if (_activated)
		deactivate();

	delete _ports;
}

Port*
NodeImpl::port(uint32_t index) const
{
	return (*_ports)[index];
}

const Plugin*
NodeImpl::plugin() const
{
	return _plugin;
}

void
NodeImpl::activate(BufferFactory& bufs)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	assert(!_activated);
	_activated = true;

	for (uint32_t p = 0; p < num_ports(); ++p) {
		PortImpl* const port = _ports->at(p);
		port->setup_buffers(bufs, port->poly());
		port->connect_buffers();
		for (uint32_t v = 0; v < _polyphony; ++v) {
			Buffer* const buf = port->buffer(v).get();
			if (buf) {
				if (port->is_a(PortType::CONTROL) || port->is_a(PortType::CV)) {
					((AudioBuffer*)buf)->set_value(
						port->value().get_float(), 0, 0);
				} else {
					buf->clear();
				}
			}
		}
	}
}

void
NodeImpl::deactivate()
{
	assert(_activated);
	_activated = false;
	for (uint32_t i = 0; i < _polyphony; ++i) {
		for (unsigned long j = 0; j < num_ports(); ++j) {
			PortImpl* const port = _ports->at(j);
			if (port->is_output() && port->buffer(i))
				port->buffer(i)->clear();
		}
	}
}

bool
NodeImpl::prepare_poly(BufferFactory& bufs, uint32_t poly)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	if (!_polyphonic)
		poly = 1;

	if (_ports)
		for (size_t i = 0; i < _ports->size(); ++i)
			_ports->at(i)->prepare_poly(bufs, poly);

	return true;
}

bool
NodeImpl::apply_poly(Raul::Maid& maid, uint32_t poly)
{
	ThreadManager::assert_thread(THREAD_PROCESS);

	if (!_polyphonic)
		poly = 1;

	_polyphony = poly;

	if (_ports)
		for (size_t i = 0; i < num_ports(); ++i)
			_ports->at(i)->apply_poly(maid, poly);

	return true;
}

void
NodeImpl::set_buffer_size(Context&       context,
                          BufferFactory& bufs,
                          LV2_URID       type,
                          uint32_t       size)
{
	if (_ports) {
		for (size_t i = 0; i < _ports->size(); ++i) {
			PortImpl* const p = _ports->at(i);
			if (p->buffer_type() == type) {
				p->set_buffer_size(context, bufs, size);
			}
		}
	}
}

void
NodeImpl::reset_input_ready()
{
	_n_inputs_ready = 0;
	_process_lock = 0;
	_input_ready.reset(0);
}

bool
NodeImpl::process_lock()
{
	return _process_lock.compare_and_exchange(0, 1);
}

void
NodeImpl::process_unlock()
{
	_process_lock = 0;
}

void
NodeImpl::wait_for_input(size_t num_providers)
{
	ThreadManager::assert_thread(THREAD_PROCESS);
	assert(_process_lock.get() == 1);

	while ((unsigned)_n_inputs_ready.get() < num_providers)
		_input_ready.wait();
}

void
NodeImpl::signal_input_ready()
{
	ThreadManager::assert_thread(THREAD_PROCESS);
	++_n_inputs_ready;
	_input_ready.post();
}

/** Prepare to run a cycle (in the audio thread)
 */
void
NodeImpl::pre_process(Context& context)
{
	ThreadManager::assert_thread(THREAD_PROCESS);

	// Mix down input ports
	for (uint32_t i = 0; i < num_ports(); ++i) {
		PortImpl* const port = _ports->at(i);
		port->pre_process(context);
		port->connect_buffers(context.offset());
	}
}

/** Prepare to run a cycle (in the audio thread)
 */
void
NodeImpl::post_process(Context& context)
{
	ThreadManager::assert_thread(THREAD_PROCESS);

	// Write output ports
	for (size_t i = 0; _ports && i < _ports->size(); ++i) {
		_ports->at(i)->post_process(context);
	}
}

void
NodeImpl::set_port_buffer(uint32_t           voice,
                          uint32_t           port_num,
                          BufferFactory::Ref buf,
                          SampleCount        offset)
{
	/*std::cout << path() << " set port " << port_num << " voice " << voice
			<< " buffer " << buf << " offset " << offset << std::endl;*/
}

} // namespace Server
} // namespace Ingen

