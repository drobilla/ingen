/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "BufferFactory.hpp"

#include "Buffer.hpp"
#include "Engine.hpp"

#include "ingen/Log.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "lv2/atom/atom.h"
#include "lv2/urid/urid.h"

#include <algorithm>

namespace ingen {
namespace server {

BufferFactory::BufferFactory(Engine& engine, URIs& uris)
	: _free_audio(nullptr)
	, _free_control(nullptr)
	, _free_sequence(nullptr)
	, _free_object(nullptr)
	, _engine(engine)
	, _uris(uris)
	, _seq_size(0)
	, _silent_buffer(nullptr)
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
	return _engine.world().forge();
}

Raul::Maid&
BufferFactory::maid()
{
	return *_engine.maid();
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
	return nframes * sizeof(Sample);
}

uint32_t
BufferFactory::audio_buffer_size() const
{
	return _engine.block_length() * sizeof(Sample);
}

uint32_t
BufferFactory::default_size(LV2_URID type) const
{
	if (type == _uris.atom_Float) {
		return sizeof(LV2_Atom_Float);
	} else if (type == _uris.atom_Sound) {
		return audio_buffer_size(_engine.block_length());
	} else if (type == _uris.atom_URID) {
		return sizeof(LV2_Atom_URID);
	} else if (type == _uris.atom_Sequence) {
		if (_seq_size == 0) {
			return _engine.sequence_size();
		} else {
			return _seq_size;
		}
	} else {
		return 0;
	}
}

Buffer*
BufferFactory::try_get_buffer(LV2_URID type)
{
	std::atomic<Buffer*>& head_ptr = free_list(type);
	Buffer*               head     = nullptr;
	Buffer*               next;
	do {
		head = head_ptr.load();
		if (!head) {
			break;
		}
		next = head->_next;
	} while (!head_ptr.compare_exchange_weak(head, next));

	return head;
}

BufferRef
BufferFactory::get_buffer(LV2_URID type,
                          LV2_URID value_type,
                          uint32_t capacity)
{
	Buffer* try_head = try_get_buffer(type);
	if (!try_head) {
		return create(type, value_type, capacity);
	}

	try_head->_next = nullptr;
	try_head->set_type(&BufferFactory::get_buffer, type, value_type);
	try_head->clear();
	return BufferRef(try_head);
}

BufferRef
BufferFactory::claim_buffer(LV2_URID type,
                            LV2_URID value_type,
                            uint32_t capacity)
{
	Buffer* try_head = try_get_buffer(type);
	if (!try_head) {
		_engine.world().log().rt_error("Failed to obtain buffer");
		return BufferRef();
	}

	try_head->_next = nullptr;
	try_head->set_type(&BufferFactory::claim_buffer, type, value_type);
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

} // namespace server
} // namespace ingen
