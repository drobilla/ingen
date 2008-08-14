/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include "NodeBase.hpp"
#include <cassert>
#include <iostream>
#include <stdint.h>
#include <raul/List.hpp>
#include <raul/Array.hpp>
#include "util.hpp"
#include "PluginImpl.hpp"
#include "ClientBroadcaster.hpp"
#include "PortImpl.hpp"
#include "PatchImpl.hpp"
#include "ObjectStore.hpp"
#include "ThreadManager.hpp"

using namespace std;

namespace Ingen {


NodeBase::NodeBase(PluginImpl* plugin, const string& name, bool polyphonic, PatchImpl* parent, SampleRate srate, size_t buffer_size)
	: NodeImpl(parent, name, polyphonic)
	, _plugin(plugin)
	, _polyphony((polyphonic && parent) ? parent->internal_polyphony() : 1)
	, _srate(srate)
	, _buffer_size(buffer_size)
	, _activated(false)
	, _traversed(false)
	, _input_ready(1)
	, _process_lock(0)
	, _n_inputs_ready(0)
	, _ports(NULL)
	, _providers(new Raul::List<NodeImpl*>())
	, _dependants(new Raul::List<NodeImpl*>())
{
	assert(_plugin);
	assert(_polyphony > 0);
	assert(_parent == NULL || (_polyphony == parent->internal_polyphony() || _polyphony == 1));
}


NodeBase::~NodeBase()
{
	if (_activated)
		deactivate();

	delete _providers;
	delete _dependants;
}

	
Port*
NodeBase::port(uint32_t index) const
{
	return (*_ports)[index];
}


const Plugin*
NodeBase::plugin() const
{
	return _plugin;
}


void
NodeBase::activate()
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);
	assert(!_activated);
	_activated = true;
}


void
NodeBase::deactivate()
{
	// FIXME: Not true witn monolithic GUI/engine
	//assert(ThreadManager::current_thread_id() == THREAD_POST_PROCESS);
	assert(_activated);
	_activated = false;
}


bool
NodeBase::prepare_poly(uint32_t poly)
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);

	if (!_polyphonic)
		return true;

	if (_ports)
		for (size_t i=0; i < _ports->size(); ++i)
			_ports->at(i)->prepare_poly(poly);

	return true;
}


bool
NodeBase::apply_poly(Raul::Maid& maid, uint32_t poly)
{
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);

	if (!_polyphonic)
		return true;

	for (size_t i=0; i < num_ports(); ++i) {
		_ports->at(i)->apply_poly(maid, poly);
		assert(_ports->at(i)->poly() == poly);
	}
	
	for (uint32_t i=0; i < num_ports(); ++i)
		for (uint32_t j=0; j < _polyphony; ++j)
			set_port_buffer(j, i, _ports->at(i)->buffer(j));
		
	return true;
}


void
NodeBase::set_buffer_size(size_t size)
{
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);

	_buffer_size = size;
	
	if (_ports)
		for (size_t i=0; i < _ports->size(); ++i)
			_ports->at(i)->set_buffer_size(size);
}
	

void
NodeBase::reset_input_ready()
{
	//cout << path() << " RESET" << endl;
	_n_inputs_ready = 0;
	_process_lock = 0;
	_input_ready.reset(0);
}


bool
NodeBase::process_lock()
{
	return _process_lock.compare_and_exchange(0, 1);
}


void
NodeBase::process_unlock()
{
	_process_lock = 0;
}


void
NodeBase::wait_for_input(size_t num_providers)
{
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);
	assert(_process_lock.get() == 1);

	while ((unsigned)_n_inputs_ready.get() < num_providers) {
		//cout << path() << " WAITING " << _n_inputs_ready.get() << endl;
		_input_ready.wait();
		//cout << path() << " CAUGHT SIGNAL" << endl;
		//++_n_inputs_ready;
	}

	//cout << path() << " READY" << endl;
}


void
NodeBase::signal_input_ready()
{
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);
	//cout << path() << " SIGNAL" << endl;
	++_n_inputs_ready;
	_input_ready.post();
}


/** Prepare to run a cycle (in the audio thread)
 */
void
NodeBase::pre_process(ProcessContext& context)
{
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);

	// Mix down any ports with multiple inputs
	for (size_t i=0; i < num_ports(); ++i)
		_ports->at(i)->pre_process(context);
	
	// Connect port buffers (FIXME: NOT NECESSARY!)
	if (polyphonic()) {
		for (uint32_t i=0; i < _polyphony; ++i) {
			for (uint32_t j=0; j < num_ports(); ++j) {
				assert(_ports->at(j)->poly() == _polyphony);
				set_port_buffer(i, j, _ports->at(j)->buffer(i));
			}
		}
	} else {
		for (uint32_t j=0; j < num_ports(); ++j)
			set_port_buffer(0, j, _ports->at(j)->buffer(0));
	}
}


/** Prepare to run a cycle (in the audio thread)
 */
void
NodeBase::post_process(ProcessContext& context)
{
	assert(ThreadManager::current_thread_id() == THREAD_PROCESS);

	/* Write output ports */
	if (_ports)
		for (size_t i=0; i < _ports->size(); ++i)
			_ports->at(i)->post_process(context);
}


} // namespace Ingen

