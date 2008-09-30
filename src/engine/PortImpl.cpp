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
#include <raul/Array.hpp>
#include <raul/Maid.hpp>
#include "PortImpl.hpp"
#include "NodeImpl.hpp"
#include "interface/DataType.hpp"
#include "AudioBuffer.hpp"
#include "EventBuffer.hpp"
#include "ProcessContext.hpp"
#include "events/SendPortActivityEvent.hpp"

using namespace std;

namespace Ingen {


PortImpl::PortImpl(NodeImpl* const node,
                   const string&   name,
                   uint32_t        index,
                   uint32_t        poly,
                   DataType        type,
                   const Atom&     value,
                   size_t          buffer_size)
	: GraphObjectImpl(node, name, (type == DataType::AUDIO || type == DataType::CONTROL))
	, _index(index)
	, _poly(poly)
	, _buffer_size(buffer_size)
	, _type(type)
	, _value(value)
	, _fixed_buffers(false)
	, _broadcast(false)
	, _set_by_user(false)
	, _last_broadcasted_value(_value.type() == Atom::FLOAT ? _value.get_float() : 0.0f) // default?
	, _context(Context::AUDIO)
	, _buffers(new Raul::Array<Buffer*>(poly))
{
	assert(node != NULL);
	assert(_poly > 0);

	allocate_buffers();
	clear_buffers();

	if (node->parent() == NULL)
		_polyphonic = false;
	else
		_polyphonic = true;
	
	if (type == DataType::EVENT)
		_broadcast = true; // send activity blips

	assert(_buffers->size() > 0);
}


PortImpl::~PortImpl()
{
	for (uint32_t i=0; i < _poly; ++i)
		delete _buffers->at(i);

	delete _buffers;
}

	
bool
PortImpl::set_polyphonic(Raul::Maid& maid, bool p)
{
	if (_type == DataType::CONTROL || _type == DataType::AUDIO)
		return GraphObjectImpl::set_polyphonic(maid, p);
	else
		return (!p);
}


bool
PortImpl::prepare_poly(uint32_t poly)
{
	if (!_polyphonic || !_parent->polyphonic())
		return true;

	/* FIXME: poly never goes down, harsh on memory.. */
	if (poly > _poly) {
		_prepared_buffers = new Raul::Array<Buffer*>(poly, *_buffers);
		for (uint32_t i = _poly; i < _prepared_buffers->size(); ++i)
			_prepared_buffers->at(i) = Buffer::create(_type, _buffer_size);
	}

	return true;
}


bool
PortImpl::apply_poly(Raul::Maid& maid, uint32_t poly)
{
	if (!_polyphonic || !_parent->polyphonic())
		return true;

	assert(poly <= _prepared_buffers->size());

	// Apply a new set of buffers from a preceding call to prepare_poly
	if (_prepared_buffers && _buffers != _prepared_buffers) {
		maid.push(_buffers);
		_buffers = _prepared_buffers;
	}

	_poly = poly;
	assert(_buffers->size() >= poly);
	assert(this->poly() == poly);

	return true;
}
	

void
PortImpl::allocate_buffers()
{
	_buffers->alloc(_poly);

	for (uint32_t i=0; i < _poly; ++i)
		_buffers->at(i) = Buffer::create(_type, _buffer_size);
}


void
PortImpl::set_buffer_size(size_t size)
{
	_buffer_size = size;

	for (uint32_t i=0; i < _poly; ++i)
		_buffers->at(i)->resize(size);

	connect_buffers();
}


void
PortImpl::connect_buffers()
{
	for (uint32_t i=0; i < _poly; ++i)
		PortImpl::parent_node()->set_port_buffer(i, _index, buffer(i));
}

	
void
PortImpl::clear_buffers()
{
	for (uint32_t i=0; i < _poly; ++i)
		buffer(i)->clear();
}
	

void
PortImpl::broadcast(ProcessContext& context)
{
	if (_broadcast) {
		if (_type == DataType::CONTROL || _type == DataType::AUDIO) {
			const Sample value = ((AudioBuffer*)buffer(0))->value_at(0);
			if (value != _last_broadcasted_value) {
				const SendPortValueEvent ev(context.engine(), context.start(), this, false, 0, value);
				context.event_sink().write(sizeof(ev), &ev);
				_last_broadcasted_value = value;
			}
		} else if (_type == DataType::EVENT) {
			if (((EventBuffer*)buffer(0))->event_count() > 0) {
				const SendPortActivityEvent ev(context.engine(), context.start(), this);
				context.event_sink().write(sizeof(ev), &ev);
			}
		}
	}
}



} // namespace Ingen
