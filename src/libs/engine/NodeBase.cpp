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
#include "Port.hpp"
#include "Patch.hpp"
#include "ObjectStore.hpp"

using namespace std;

namespace Ingen {


NodeBase::NodeBase(const PluginImpl* plugin, const string& name, bool polyphonic, Patch* parent, SampleRate srate, size_t buffer_size)
: NodeImpl(parent, name, polyphonic),
  _plugin(plugin),
  _polyphony((polyphonic && parent) ? parent->internal_poly() : 1),
  _srate(srate),
  _buffer_size(buffer_size),
  _activated(false),
  _traversed(false),
  _input_ready(1),
  _process_lock(0),
  _n_inputs_ready(0),
  _ports(NULL),
  _providers(new Raul::List<NodeImpl*>()),
  _dependants(new Raul::List<NodeImpl*>())
{
	assert(_plugin);
	assert(_polyphony > 0);
	assert(_parent == NULL || (_polyphony == parent->internal_poly() || _polyphony == 1));
}


NodeBase::~NodeBase()
{
	assert(!_activated);

	delete _providers;
	delete _dependants;
	
	if (_ports)
		for (uint32_t i=0; i < num_ports(); ++i)
			delete _ports->at(i);
}


void
NodeBase::activate()
{
	assert(!_activated);
	_activated = true;
}


void
NodeBase::deactivate()
{
	assert(_activated);
	_activated = false;
}


bool
NodeBase::prepare_poly(uint32_t poly)
{
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
	if (!_polyphonic)
		return true;

	if (_ports)
		for (size_t i=0; i < _ports->size(); ++i)
			_ports->at(i)->apply_poly(maid, poly);
	
	return true;
}


void
NodeBase::set_buffer_size(size_t size)
{
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
	//cout << path() << " SIGNAL" << endl;
	++_n_inputs_ready;
	_input_ready.post();
}


/** Prepare to run a cycle (in the audio thread)
 */
void
NodeBase::pre_process(ProcessContext& context)
{
	// Mix down any ports with multiple inputs
	if (_ports)
		for (size_t i=0; i < _ports->size(); ++i)
			_ports->at(i)->pre_process(context);
}


/** Prepare to run a cycle (in the audio thread)
 */
void
NodeBase::post_process(ProcessContext& context)
{
	/* Write output ports */
	if (_ports)
		for (size_t i=0; i < _ports->size(); ++i)
			_ports->at(i)->post_process(context);
}


} // namespace Ingen

