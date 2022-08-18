/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_BUFFER_HPP
#define INGEN_ENGINE_BUFFER_HPP

#include "BufferFactory.hpp"
#include "BufferRef.hpp"
#include "PortType.hpp"
#include "server.h"
#include "types.hpp"

#include "ingen/URIs.hpp"
#include "lv2/atom/atom.h"
#include "lv2/urid/urid.h"

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace ingen {

class Atom;

namespace server {

class RunContext;

class INGEN_SERVER_API Buffer
{
public:
	Buffer(BufferFactory& bufs,
	       LV2_URID       type,
	       LV2_URID       value_type,
	       uint32_t       capacity,
	       bool           external = false,
	       void*          buf = nullptr);

	Buffer(const Buffer&) = delete;
	Buffer& operator=(const Buffer&) = delete;

	void clear();
	void resize(uint32_t capacity);
	void copy(const RunContext& ctx, const Buffer* src);
	void prepare_write(RunContext& ctx);

	void*       port_data(PortType port_type, SampleCount offset);
	const void* port_data(PortType port_type, SampleCount offset) const;

	LV2_URID type()       const { return _type; }
	LV2_URID value_type() const { return _value_type; }
	uint32_t capacity()   const { return _capacity; }
	uint32_t size()       const {
		return is_audio() ? _capacity : sizeof(LV2_Atom) + get<LV2_Atom>()->size;
	}

	using GetFn = BufferRef (BufferFactory::*)(LV2_URID, LV2_URID, uint32_t);

	/** Set the buffer type and optional value type for this buffer.
	 *
	 * @param get_func Called to get auxiliary buffers if necessary.
	 * @param type Type of buffer.
	 * @param value_type Type of values in buffer if applicable (for sequences).
	 */
	void set_type(GetFn get_func, LV2_URID type, LV2_URID value_type);

	bool is_audio() const {
		return _type == _factory.uris().atom_Sound;
	}

	bool is_control() const {
		return _type == _factory.uris().atom_Float;
	}

	bool is_sequence() const {
		return _type == _factory.uris().atom_Sequence;
	}

	/// Audio or float buffers only
	const Sample* samples() const {
		if (is_control()) {
			return static_cast<const Sample*>(
			    LV2_ATOM_BODY_CONST(get<LV2_Atom_Float>()));
		}

		if (is_audio()) {
			return static_cast<const Sample*>(_buf);
		}

		return nullptr;
	}

	/// Audio buffers only
	Sample* samples() {
		if (is_control()) {
			return static_cast<Sample*>(LV2_ATOM_BODY(get<LV2_Atom_Float>()));
		}

		if (is_audio()) {
			return static_cast<Sample*>(_buf);
		}

		return nullptr;
	}

	/// Numeric buffers only
	Sample value_at(SampleCount offset) const {
		if (is_audio() || is_control()) {
			return samples()[offset];
		}

		if (_value_buffer) {
			return reinterpret_cast<const LV2_Atom_Float*>(value())->body;
		}

		return 0.0f;
	}

	void
	set_block(const Sample val, const SampleCount start, const SampleCount end)
	{
		if (is_sequence()) {
			append_event(start, sizeof(val), _factory.uris().atom_Float,
			             reinterpret_cast<const uint8_t*>(
				             static_cast<const float*>(&val)));
			_value_buffer->get<LV2_Atom_Float>()->body = val;
			return;
		}

		assert(is_audio() || is_control());
		assert(end <= _capacity / sizeof(Sample));
		// Note: Do not change this without ensuring GCC can still vectorize it
		Sample* const buf = samples() + start;
		for (SampleCount i = 0; i < (end - start); ++i) {
			buf[i] = val;
		}
	}

	void
	add_block(const Sample val, const SampleCount start, const SampleCount end)
	{
		assert(is_audio() || is_control());
		assert(end <= _capacity / sizeof(Sample));
		// Note: Do not change this without ensuring GCC can still vectorize it
		Sample* const buf = samples() + start;
		for (SampleCount i = 0; i < (end - start); ++i) {
			buf[i] += val;
		}
	}

	void write_block(const Sample      val,
	                 const SampleCount start,
	                 const SampleCount end,
	                 const bool        add)
	{
		if (add) {
			add_block(val, start, end);
		} else {
			set_block(val, start, end);
		}
	}

	/// Audio buffers only
	float peak(const RunContext& ctx) const;

	/// Sequence buffers only
	void prepare_output_write(RunContext& ctx);

	/// Sequence buffers only
	bool append_event(int64_t        frames,
	                  uint32_t       size,
	                  uint32_t       type,
	                  const uint8_t* data);

	/// Sequence buffers only
	bool append_event(int64_t frames, const LV2_Atom* body);

	/// Sequence buffers only
	bool append_event_buffer(const Buffer* buf);

	/// Value buffer for numeric sequences
	BufferRef value_buffer() { return _value_buffer; }

	/// Return the current value
	const LV2_Atom* value() const;

	/// Set/initialise current value in value buffer
	void set_value(const Atom& value);

	/// Return offset of the first value change after `offset`
	SampleCount next_value_offset(SampleCount offset, SampleCount end) const;

	/// Update value buffer to value as of offset
	void update_value_buffer(SampleCount offset);

	/// Set/add to audio buffer from the Sequence of Float in `src`
	void render_sequence(const RunContext& ctx, const Buffer* src, bool add);

	void set_capacity(uint32_t capacity) { _capacity = capacity; }

	void set_buffer(void* buf) { assert(_external); _buf = buf; }

	static void* aligned_alloc(size_t size);

	template<typename T> const T* get() const { return reinterpret_cast<const T*>(_buf); }
	template<typename T> T*       get()       { return reinterpret_cast<T*>(_buf); }

	void ref() { ++_refs; }

	void deref() {
		if ((--_refs) == 0) {
			recycle();
		}
	}

private:
	friend class BufferFactory;
	~Buffer();

	void recycle();

	BufferFactory& _factory;

	// NOLINTNEXTLINE(clang-analyzer-webkit.NoUncountedMemberChecker)
	Buffer* _next{nullptr}; ///< Intrusive linked list for BufferFactory

	void*                 _buf; ///< Actual buffer memory
	BufferRef             _value_buffer; ///< Value buffer for numeric sequences
	int64_t               _latest_event{0};
	LV2_URID              _type;
	LV2_URID              _value_type;
	uint32_t              _capacity;
	std::atomic<unsigned> _refs{0}; ///< Intrusive reference count
	bool                  _external; ///< Buffer is externally allocated
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_BUFFER_HPP
