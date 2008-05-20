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

#include <iostream>
#include <cstdlib>
#include <cassert>
#include "util.hpp"
#include "DuplexPort.hpp"
#include "ConnectionImpl.hpp"
#include "OutputPort.hpp"
#include "NodeImpl.hpp"
#include "ProcessContext.hpp"
#include "EventBuffer.hpp"

using namespace std;

namespace Ingen {


DuplexPort::DuplexPort(NodeImpl* parent, const string& name, uint32_t index, uint32_t poly, DataType type, const Atom& value, size_t buffer_size, bool is_output)
	: PortImpl(parent, name, index, poly, type, value, buffer_size)
	, InputPort(parent, name, index, poly, type, value, buffer_size)
	, OutputPort(parent, name, index, poly, type, value, buffer_size)
	, _is_output(is_output)
{
	assert(PortImpl::_parent == parent);
}


void
DuplexPort::pre_process(ProcessContext& context)
{
	// <BrainHurt>
		
	/*cerr << endl << "{ duplex pre" << endl;
	cerr << path() << " duplex pre: fixed buffers: " << fixed_buffers() << endl;
	cerr << path() << " duplex pre: buffer: " << buffer(0) << endl;
	cerr << path() << " duplex pre: is_output: " << _is_output << " { " << endl;*/
	
	/*if (type() == DataType::EVENT) 
		for (uint32_t i=0; i < _poly; ++i)
			cerr << path() << " (" << buffer(i) << ") # events: "
				<< ((EventBuffer*)buffer(i))->event_count()
				<< ", joined: " << _buffers->at(i)->is_joined() << endl;*/
	
	if (_is_output) {

		for (uint32_t i=0; i < _poly; ++i)
			if (!_buffers->at(i)->is_joined())
				_buffers->at(i)->prepare_write(context.nframes());

	} else {

		for (uint32_t i=0; i < _poly; ++i)
			_buffers->at(i)->prepare_read(context.nframes());

		broadcast(context);
	}

	//cerr << "} duplex pre " << path() << endl;

	// </BrainHurt>
}


void
DuplexPort::post_process(ProcessContext& context)
{
	// <BrainHurt>
	
	/*cerr << endl << "{ duplex post" << endl;
	cerr << path() << " duplex post: fixed buffers: " << fixed_buffers() << endl;
	cerr << path() << " duplex post: buffer: " << buffer(0) << endl;
	cerr << path() << " duplex post: is_output: " << _is_output << " { " << endl;
	
	if (type() == DataType::EVENT) 
		for (uint32_t i=0; i < _poly; ++i)
			cerr << path() << " (" << buffer(i) << ") # events: "
				<< ((EventBuffer*)buffer(i))->event_count()
				<< ", joined: " << _buffers->at(i)->is_joined() << endl;*/

	if (_is_output) {
		InputPort::pre_process(context); // Mix down inputs
		broadcast(context);
	}
	
	//cerr << "} duplex post " << path() << endl;
	
	// </BrainHurt>
}


} // namespace Ingen

