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

#include <algorithm>
#include <iostream>
#include "shared/LV2URIMap.hpp"
#include "AudioBuffer.hpp"
#include "EventBuffer.hpp"
#include "ObjectBuffer.hpp"
#include "BufferFactory.hpp"
#include "Engine.hpp"
#include "Driver.hpp"
#include "ThreadManager.hpp"

namespace Ingen {

using namespace Shared;

BufferFactory::BufferFactory(Engine& engine, SharedPtr<Shared::LV2URIMap> map)
	: _engine(engine)
	, _map(map)
{
}

struct BufferDeleter {
	BufferDeleter(BufferFactory& bf) : _factory(bf) {}
	void operator()(void* ptr) {
		_factory.recycle((Buffer*)ptr);
	}
	BufferFactory& _factory;
};


BufferFactory::Ref
BufferFactory::get(Shared::PortType type, size_t size, bool force_create)
{
	Raul::AtomicPtr<Buffer>& head_ptr = free_list(type);
	Buffer* try_head = NULL;

	if (!force_create) {
		Buffer* next;
		do {
			try_head = head_ptr.get();
			if (!try_head)
				break;
			next = try_head->_next;
		} while (!head_ptr.compare_and_exchange(try_head, next));
	}

	if (!try_head) {
		if (ThreadManager::current_thread_id() != THREAD_PROCESS) {
			return create(type, size);
		} else {
			cerr << "ERROR: Failed to obtain buffer" << endl;
			return Ref();
		}
	}

	try_head->_next = NULL;
	return Ref(try_head);
}


BufferFactory::Ref
BufferFactory::create(Shared::PortType type, size_t size)
{
	assert(ThreadManager::current_thread_id() != THREAD_PROCESS);

	Buffer* buffer = NULL;

	if (type.is_control()) {
		if (size == 0)
			size = sizeof(LV2_Object) + sizeof(float);
		AudioBuffer* ret = new AudioBuffer(*this, type, size);
		ret->object()->type = _map->object_class_vector;
		((LV2_Vector_Body*)ret->object()->body)->elem_type = _map->object_class_float32;
		buffer = ret;
	} else if (type.is_audio()) {
		if (size == 0)
			size = sizeof(LV2_Object) + sizeof(LV2_Vector_Body)
				+ _engine.driver()->buffer_size() * sizeof(float);
		AudioBuffer* ret = new AudioBuffer(*this, type, size);
		ret->object()->type = _map->object_class_float32;
		buffer = ret;
	} else if (type.is_events()) {
		if (size == 0)
			size = _engine.driver()->buffer_size() * 4; // FIXME
		buffer = new EventBuffer(*this, size);
	} else if (type.is_value() || type.is_message()) {
		if (size == 0)
			size = 32; // FIXME
		buffer = new ObjectBuffer(*this, std::max(size, sizeof(LV2_Object) + sizeof(void*)));
	} else {
		cout << "ERROR: Failed to create buffer of unknown type" << endl;
		return Ref();
	}

	return Ref(buffer);
}

void
BufferFactory::recycle(Buffer* buf)
{
	Raul::AtomicPtr<Buffer>& head_ptr = free_list(buf->type());
	Buffer* try_head;
	do {
		try_head = head_ptr.get();
		buf->_next = try_head;
	} while (!head_ptr.compare_and_exchange(try_head, buf));
}


} // namespace Ingen
