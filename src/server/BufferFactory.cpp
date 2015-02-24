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

#include "ingen/Log.hpp"
#include "ingen/URIs.hpp"

#include "Buffer.hpp"
#include "BufferFactory.hpp"
#include "Driver.hpp"
#include "Engine.hpp"

namespace Ingen {
namespace Server {

BufferFactory::BufferFactory(Engine& engine, URIs& uris)
	: _free_audio(NULL)
	, _free_control(NULL)
	, _free_sequence(NULL)
	, _free_object(NULL)
	, _engine(engine)
	, _uris(uris)
	, _seq_size(0)
	, _silent_buffer(NULL)
{
}

BufferFactory::~BufferFactory()
{
	_silent_buffer.reset();
	free_list(_free_audio.load());
	free_list(_free_control.load());
	free_list(_free_sequence.load());
	free_list(_free_object.load());
}

Forge&
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
	_silent_buffer = create(_uris.atom_Sound, audio_buffer_size(block_length));
}

uint32_t
BufferFactory::audio_buffer_size(SampleCount nframes)
{
	return sizeof(LV2_Atom_Vector) + (nframes * sizeof(float));
}

uint32_t
BufferFactory::default_size(LV2_URID type) const
{
	if (type == _uris.atom_Float) {
		return sizeof(LV2_Atom_Float);
	} else if (type == _uris.atom_Sound) {
		return audio_buffer_size(_engine.driver()->block_length());
	} else if (type == _uris.atom_URID) {
		return sizeof(LV2_Atom_URID);
	} else if (type == _uris.atom_Sequence) {
		if (_seq_size == 0) {
			return _engine.driver()->seq_size();
		} else {
			return _seq_size;
		}
	} else {
		return 0;
	}
}

BufferRef
BufferFactory::get_buffer(LV2_URID type,
                          LV2_URID value_type,
                          uint32_t capacity,
                          bool     real_time,
                          bool     force_create)
{
	std::atomic<Buffer*>& head_ptr = free_list(type);
	Buffer*               try_head = NULL;

	if (!force_create) {
		Buffer* next;
		do {
			try_head = head_ptr.load();
			if (!try_head)
				break;
			next = try_head->_next;
		} while (!head_ptr.compare_exchange_weak(try_head, next));
	}

	if (!try_head) {
		if (!real_time) {
			return create(type, value_type, capacity);
		} else {
			_engine.world()->log().error("Failed to obtain buffer");
			return BufferRef();
		}
	}

	try_head->_next = NULL;
	try_head->set_type(type, value_type);
	return BufferRef(try_head);
}

BufferRef
BufferFactory::silent_buffer()
{
	return _silent_buffer;
}

BufferRef
BufferFactory::create(LV2_URID type, LV2_URID value_type, uint32_t capacity)
{
	if (capacity == 0) {
		capacity = default_size(type);
	} else if (type == _uris.atom_Float) {
		capacity = std::max(capacity, (uint32_t)sizeof(LV2_Atom_Float));
	} else if (type == _uris.atom_Sound) {
		capacity = std::max(capacity, default_size(_uris.atom_Sound));
	}

	return BufferRef(new Buffer(*this, type, value_type, capacity));
}

void
BufferFactory::recycle(Buffer* buf)
{
	std::atomic<Buffer*>& head_ptr = free_list(buf->type());
	Buffer* try_head;
	do {
		try_head = head_ptr.load();
		buf->_next = try_head;
	} while (!head_ptr.compare_exchange_weak(try_head, buf));
}

} // namespace Server
} // namespace Ingen
