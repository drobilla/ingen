/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>

#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/shared/URIs.hpp"
#include "raul/log.hpp"

#include "AudioBuffer.hpp"
#include "BufferFactory.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "ThreadManager.hpp"

using namespace Raul;

namespace Ingen {
namespace Server {

static const size_t EVENT_BYTES_PER_FRAME = 4; // FIXME

BufferFactory::BufferFactory(Engine&                        engine,
                             SharedPtr<Ingen::Shared::URIs> uris)
	: _engine(engine)
	, _uris(uris)
	, _silent_buffer(NULL)
{
	assert(_uris);
}

BufferFactory::~BufferFactory()
{
	_silent_buffer.reset();
	free_list(_free_audio.get());
	free_list(_free_control.get());
	free_list(_free_object.get());
}

Ingen::Forge&
BufferFactory::forge()
{
	return _engine.world()->forge();
}

void
BufferFactory::free_list(Buffer* head)
{
	while (head) {
		Buffer* next = head->_next;
		delete head;
		head = next;
	}
}

void
BufferFactory::set_block_length(SampleCount block_length)
{
	_silent_buffer = create(_uris->atom_Sound, audio_buffer_size(block_length));
}

uint32_t
BufferFactory::audio_buffer_size(SampleCount nframes)
{
	return sizeof(LV2_Atom_Vector) + (nframes * sizeof(float));
}

uint32_t
BufferFactory::default_buffer_size(LV2_URID type)
{
	if (type == _uris->atom_Float) {
		return sizeof(LV2_Atom_Float);
	} else if (type == _uris->atom_Sound) {
		return audio_buffer_size(_engine.driver()->block_length());
	} else if (type == _uris->atom_Sequence) {
		return _engine.driver()->block_length() * EVENT_BYTES_PER_FRAME;
	} else {
		return 0;
	}
}

BufferRef
BufferFactory::get(LV2_URID type, uint32_t capacity, bool force_create)
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
		if (!ThreadManager::thread_is(THREAD_PROCESS)) {
			return create(type, capacity);
		} else {
			assert(false);
			error << "Failed to obtain buffer" << endl;
			return BufferRef();
		}
	}

	try_head->_next = NULL;
	return BufferRef(try_head);
}

BufferRef
BufferFactory::silent_buffer()
{
	return _silent_buffer;
}

BufferRef
BufferFactory::create(LV2_URID type, uint32_t capacity)
{
	ThreadManager::assert_not_thread(THREAD_PROCESS);

	Buffer* buffer = NULL;

	if (capacity == 0) {
		capacity = default_buffer_size(type);
	}

	if (type == _uris->atom_Float) {
		assert(capacity >= sizeof(LV2_Atom_Float));
		buffer = new AudioBuffer(*this, type, capacity);
	} else if (type == _uris->atom_Sound) {
		assert(capacity >= default_buffer_size(_uris->atom_Sound));
		buffer = new AudioBuffer(*this, type, capacity);
	} else {
		buffer = new Buffer(*this, type, capacity);
	}

	buffer->atom()->type = type;

	assert(buffer);
	return BufferRef(buffer);
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

} // namespace Server
} // namespace Ingen
