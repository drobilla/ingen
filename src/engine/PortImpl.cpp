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

#include "raul/Array.hpp"
#include "raul/Maid.hpp"
#include "shared/LV2URIMap.hpp"
#include "contexts.lv2/contexts.h"
#include "interface/PortType.hpp"
#include "events/SendPortValue.hpp"
#include "events/SendPortActivity.hpp"
#include "AudioBuffer.hpp"
#include "BufferFactory.hpp"
#include "Engine.hpp"
#include "EventBuffer.hpp"
#include "LV2Object.hpp"
#include "NodeImpl.hpp"
#include "ObjectBuffer.hpp"
#include "PortImpl.hpp"
#include "ThreadManager.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Shared;


PortImpl::PortImpl(BufferFactory&      bufs,
                   NodeImpl* const     node,
                   const Raul::Symbol& name,
                   uint32_t            index,
                   uint32_t            poly,
                   PortType            type,
                   const Atom&         value,
                   size_t              buffer_size)
	: GraphObjectImpl(node, name)
	, _bufs(bufs)
	, _index(index)
	, _poly(poly)
	, _buffer_size(buffer_size)
	, _type(type)
	, _value(value)
	, _broadcast(false)
	, _set_by_user(false)
	, _last_broadcasted_value(value)
	, _context(Context::AUDIO)
	, _buffers(new Array<BufferFactory::Ref>(static_cast<size_t>(poly)))
	, _prepared_buffers(NULL)
{
	assert(node != NULL);
	assert(_poly > 0);

	if (_buffer_size == 0)
		_buffer_size = bufs.default_buffer_size(type);

	const LV2URIMap& uris = Shared::LV2URIMap::instance();
	add_property(uris.rdf_type,  type.uri());
	set_property(uris.lv2_index, Atom((int32_t)index));
	set_context(_context);

	if (type == PortType::EVENTS)
		_broadcast = true; // send activity blips
}


PortImpl::~PortImpl()
{
	delete _buffers;
}


Raul::Array<BufferFactory::Ref>*
PortImpl::set_buffers(Raul::Array<BufferFactory::Ref>* buffers)
{
	ThreadManager::assert_thread(THREAD_PROCESS);

	Raul::Array<BufferFactory::Ref>* ret = NULL;
	if (buffers != _buffers) {
		ret = _buffers;
		_buffers = buffers;
	}

	connect_buffers();

	return ret;
}


bool
PortImpl::prepare_poly(BufferFactory& bufs, uint32_t poly)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	if (_type != PortType::CONTROL && _type != PortType::AUDIO)
		return false;

	if (_prepared_buffers && _prepared_buffers->size() < poly) {
		delete _prepared_buffers;
		_prepared_buffers = NULL;
	}

	if (!_prepared_buffers)
		_prepared_buffers = new Array<BufferFactory::Ref>(poly, *_buffers, NULL);

	get_buffers(bufs, _prepared_buffers, poly);
	assert(prepared_poly() == poly);

	return true;
}


bool
PortImpl::apply_poly(Maid& maid, uint32_t poly)
{
	ThreadManager::assert_thread(THREAD_PROCESS);
	if (_type != PortType::CONTROL && _type != PortType::AUDIO)
		return false;

	assert(poly <= _prepared_buffers->size());

	_poly = poly;

	// Apply a new set of buffers from a preceding call to prepare_poly
	maid.push(set_buffers(_prepared_buffers));
	assert(_buffers == _prepared_buffers);
	_prepared_buffers = NULL;

	assert(_buffers->size() >= poly);
	assert(this->poly() == poly);
	assert(!_prepared_buffers);

	return true;
}


void
PortImpl::set_buffer_size(Context& context, BufferFactory& bufs, size_t size)
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
PortImpl::recycle_buffers()
{
	for (uint32_t v = 0; v < _poly; ++v)
		_buffers->at(v) = NULL;
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

	if (val.is_valid() && (force || val != _last_broadcasted_value)) {
		_last_broadcasted_value = val;
		const Events::SendPortValue ev(context.engine(), context.start(), this, true, 0, val);
		context.event_sink().write(sizeof(ev), &ev);
	}
}


void
PortImpl::set_context(Context::ID c)
{
	const LV2URIMap& uris = Shared::LV2URIMap::instance();
	_context = c;
	switch (c) {
	case Context::AUDIO:
		set_property(uris.ctx_context, uris.ctx_AudioContext);
		break;
	case Context::MESSAGE:
		set_property(uris.ctx_context, uris.ctx_MessageContext);
		break;
	}
}


} // namespace Ingen
