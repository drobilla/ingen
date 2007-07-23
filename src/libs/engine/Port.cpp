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

#include "Port.h"
#include "Node.h"
#include "DataType.h"
#include "Buffer.h"
#include "BufferFactory.h"

namespace Ingen {


// FIXME: Make these actually URIs..
const char* const DataType::type_uris[4] = { "UNKNOWN", "FLOAT", "MIDI", "OSC" };


Port::Port(Node* const node, const string& name, size_t index, size_t poly, DataType type, size_t buffer_size)
	: GraphObject(node, name)
	, _index(index)
	, _poly(poly)
	, _type(type)
	, _buffer_size(buffer_size)
	, _fixed_buffers(false)
{
	assert(node != NULL);
	assert(_poly > 0);

	allocate_buffers();
	clear_buffers();

	assert(_buffers.size() > 0);
}


Port::~Port()
{
	for (size_t i=0; i < _poly; ++i)
		delete _buffers.at(i);
}


void
Port::allocate_buffers()
{
	_buffers.alloc(_poly);

	for (size_t i=0; i < _poly; ++i)
		_buffers.at(i) = BufferFactory::create(_type, _buffer_size);
}


void
Port::set_buffer_size(size_t size)
{
	_buffer_size = size;

	for (size_t i=0; i < _poly; ++i)
		_buffers.at(i)->resize(size);

	connect_buffers();
}


void
Port::connect_buffers()
{
	for (size_t i=0; i < _poly; ++i)
		Port::parent_node()->set_port_buffer(i, _index, _buffers.at(i));
}

	
void
Port::clear_buffers()
{
	for (size_t i=0; i < _poly; ++i)
		_buffers.at(i)->clear();
}


} // namespace Ingen
