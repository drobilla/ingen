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
#include "Plugin.hpp"
#include "ClientBroadcaster.hpp"
#include "Port.hpp"
#include "Patch.hpp"
#include "ObjectStore.hpp"

using std::cout; using std::cerr; using std::endl;

namespace Ingen {


NodeBase::NodeBase(const Plugin* plugin, const string& name, size_t poly, Patch* parent, SampleRate srate, size_t buffer_size)
: Node(parent, name),
  _plugin(plugin),
  _poly(poly),
  _srate(srate),
  _buffer_size(buffer_size),
  _activated(false),
  _ports(NULL),
  _traversed(false),
  _providers(new Raul::List<Node*>()),
  _dependants(new Raul::List<Node*>())
{
	assert(_plugin);
	assert(_poly > 0);
	assert(_parent == NULL || (_poly == parent->internal_poly() || _poly == 1));
}


NodeBase::~NodeBase()
{
	assert(!_activated);

	delete _providers;
	delete _dependants;
	
	for (size_t i=0; i < num_ports(); ++i)
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


void
NodeBase::set_buffer_size(size_t size)
{
	_buffer_size = size;
	
	if (_ports)
		for (size_t i=0; i < _ports->size(); ++i)
			_ports->at(i)->set_buffer_size(size);
}


/** Prepare to run a cycle (in the audio thread)
 */
void
NodeBase::pre_process(SampleCount nframes, FrameTime start, FrameTime end)
{
	assert(_activated);
	// Mix down any ports with multiple inputs
	for (size_t i=0; i < _ports->size(); ++i)
		_ports->at(i)->pre_process(nframes, start, end);
}


/** Prepare to run a cycle (in the audio thread)
 */
void
NodeBase::post_process(SampleCount nframes, FrameTime start, FrameTime end)
{
	assert(_activated);
	// Prepare any output ports for reading (MIDI)
	for (size_t i=0; i < _ports->size(); ++i)
		_ports->at(i)->post_process(nframes, start, end);
}


} // namespace Ingen

