/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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
#include "raul/log.hpp"
#include "ingen/shared/LV2URIMap.hpp"
#include "AudioBuffer.hpp"
#include "EventBuffer.hpp"
#include "ObjectBuffer.hpp"
#include "BufferFactory.hpp"
#include "Engine.hpp"
#include "Driver.hpp"
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
	free_list(_free_event.get());
	free_list(_free_object.get());
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
	_silent_buffer = create(PortType::AUDIO, audio_buffer_size(block_length));
}

size_t
BufferFactory::audio_buffer_size(SampleCount nframes)
{
	return sizeof(LV2_Atom) + sizeof(LV2_Atom_Vector) + (nframes * sizeof(float));
}

size_t
BufferFactory::default_buffer_size(PortType type)
{
	switch (type.symbol()) {
		case PortType::AUDIO:
			return audio_buffer_size(_engine.driver()->block_length());
		case PortType::CONTROL:
			return sizeof(LV2_Atom) + sizeof(float);
		case PortType::EVENTS:
			return _engine.driver()->block_length() * EVENT_BYTES_PER_FRAME;
		default:
			return 1024; // Who knows
	}
}

BufferFactory::Ref
BufferFactory::get(PortType type, size_t size, bool force_create)
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
			return create(type, size);
		} else {
			assert(false);
			error << "Failed to obtain buffer" << endl;
			return Ref();
		}
	}

	try_head->_next = NULL;
	return Ref(try_head);
}

BufferFactory::Ref
BufferFactory::silent_buffer()
{
	return _silent_buffer;
}

BufferFactory::Ref
BufferFactory::create(PortType type, size_t size)
{
	ThreadManager::assert_not_thread(THREAD_PROCESS);

	Buffer* buffer = NULL;

	if (size == 0)
		size = default_buffer_size(type);

	if (type.is_control()) {
		AudioBuffer* ret = new AudioBuffer(*this, type, size);
		ret->atom()->type = _uris->atom_Vector.id;
		((LV2_Atom_Vector*)ret->atom()->body)->elem_type = _uris->atom_Float32.id;
		buffer = ret;
	} else if (type.is_audio()) {
		AudioBuffer* ret = new AudioBuffer(*this, type, size);
		ret->atom()->type = _uris->atom_Float32.id;
		buffer = ret;
	} else if (type.is_events()) {
		buffer = new EventBuffer(*this, size);
	} else if (type.is_value() || type.is_message()) {
		buffer = new ObjectBuffer(*this, std::max(size, sizeof(LV2_Atom) + sizeof(void*)));
	} else {
		error << "Failed to create buffer of unknown type" << endl;
		return Ref();
	}

	assert(buffer);
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

} // namespace Server
} // namespace Ingen
