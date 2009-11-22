/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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
#include "raul/Array.hpp"
#include "raul/Maid.hpp"
#include "contexts.lv2/contexts.h"
#include "interface/PortType.hpp"
#include "events/SendPortValue.hpp"
#include "events/SendPortActivity.hpp"
#include "AudioBuffer.hpp"
#include "EventBuffer.hpp"
#include "Engine.hpp"
#include "BufferFactory.hpp"
#include "LV2Object.hpp"
#include "NodeImpl.hpp"
#include "ObjectBuffer.hpp"
#include "PortImpl.hpp"
#include "ProcessContext.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Shared;


PortImpl::PortImpl(BufferFactory&  bufs,
                   NodeImpl* const node,
                   const string&   name,
                   uint32_t        index,
                   uint32_t        poly,
                   PortType        type,
                   const Atom&     value,
                   size_t          buffer_size)
	: GraphObjectImpl(node, name, (type == PortType::AUDIO || type == PortType::CONTROL))
	, _bufs(bufs)
	, _index(index)
	, _poly(poly)
	, _buffer_size(buffer_size)
	, _type(type)
	, _value(value)
	, _broadcast(false)
	, _set_by_user(false)
	, _last_broadcasted_value(_value.type() == Atom::FLOAT ? _value.get_float() : 0.0f) // default?
	, _context(Context::AUDIO)
	, _buffers(new Array< SharedPtr<Buffer> >(poly))
	, _prepared_buffers(NULL)
{
	assert(node != NULL);
	assert(_poly > 0);

	_buffers->alloc(_poly);
	for (uint32_t v = 0; v < _poly; ++v)
		_buffers->at(v) = bufs.get(_type, _buffer_size);

	_prepared_buffers = _buffers;

	if (node->parent() == NULL)
		_polyphonic = false;
	else
		_polyphonic = true;

	add_property("rdf:type",    Atom(Atom::URI, type.uri()));
	set_context(_context);

	if (type == PortType::EVENTS)
		_broadcast = true; // send activity blips

	assert(_buffers->size() > 0);
}


PortImpl::~PortImpl()
{
	for (uint32_t v = 0; v < _poly; ++v)
		_buffers->at(v).reset();

	delete _buffers;
}


bool
PortImpl::set_polyphonic(Maid& maid, bool p)
{
	if (_type == PortType::CONTROL || _type == PortType::AUDIO)
		return GraphObjectImpl::set_polyphonic(maid, p);
	else
		return (!p);
}


bool
PortImpl::prepare_poly(BufferFactory& bufs, uint32_t poly)
{
	if (!_polyphonic || !_parent->polyphonic())
		return true;

	/* FIXME: poly never goes down, harsh on memory.. */
	if (poly > _poly) {
		_prepared_buffers = new Array< SharedPtr<Buffer> >(poly, *_buffers);
		for (uint32_t i = _poly; i < _prepared_buffers->size(); ++i)
			_prepared_buffers->at(i) = bufs.get(_type, _buffer_size);
	}

	return true;
}


bool
PortImpl::apply_poly(Maid& maid, uint32_t poly)
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
PortImpl::set_buffer_size(BufferFactory& bufs, size_t size)
{
	_buffer_size = size;

	for (uint32_t v = 0; v < _poly; ++v)
		_buffers->at(v)->resize(size);

	connect_buffers();
}


void
PortImpl::connect_buffers()
{
	for (uint32_t v = 0; v < _poly; ++v)
		PortImpl::parent_node()->set_port_buffer(v, _index, buffer(v));
}


void
PortImpl::clear_buffers()
{
	for (uint32_t v = 0; v < _poly; ++v)
		buffer(v)->clear();
}


void
PortImpl::broadcast_value(Context& context, bool force)
{
	Raul::Atom val;
	switch (_type.symbol()) {
	case PortType::UNKNOWN:
		break;
	case PortType::AUDIO:
	case PortType::CONTROL:
		val = ((AudioBuffer*)buffer(0).get())->value_at(0);
		break;
	case PortType::EVENTS:
		if (((EventBuffer*)buffer(0).get())->event_count() > 0) {
			const Events::SendPortActivity ev(context.engine(), context.start(), this);
			context.event_sink().write(sizeof(ev), &ev);
		}
		break;
	case PortType::VALUE:
	case PortType::MESSAGE:
		LV2Object::to_atom(context.engine().world(), ((ObjectBuffer*)buffer(0).get())->object(), val);
		break;
	}

	if (val.type() == Atom::FLOAT && (force || val != _last_broadcasted_value)) {
		_last_broadcasted_value = val;
		const Events::SendPortValue ev(context.engine(), context.start(), this, false, 0, val);
		context.event_sink().write(sizeof(ev), &ev);
	}

}


void
PortImpl::set_context(Context::ID c)
{
	_context = c;
	switch (c) {
	case Context::AUDIO:
		set_property("ctx:context", Atom(Atom::URI, "ctx:AudioContext"));
		break;
	case Context::MESSAGE:
		set_property("ctx:context", Atom(Atom::URI, "ctx:MessageContext"));
		break;
	}
}


} // namespace Ingen
