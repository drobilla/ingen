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

#include <atomic>
#include <cassert>

#include <boost/utility.hpp>

#include "ingen/types.hpp"
#include "ingen/ingen.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "raul/Deletable.hpp"

#include "BufferFactory.hpp"
#include "PortType.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {

class BufferFactory;
class Engine;
class RunContext;

class INGEN_API Buffer : public boost::noncopyable
{
public:
	Buffer(BufferFactory& bufs,
	       LV2_URID       type,
	       LV2_URID       value_type,
	       uint32_t       capacity,
	       bool           external = false,
	       void*          buf = NULL);

	void clear();
	void resize(uint32_t size);
	void copy(const RunContext& context, const Buffer* src);
	void prepare_write(RunContext& context);

	void*       port_data(PortType port_type, SampleCount offset);
	const void* port_data(PortType port_type, SampleCount offset) const;

	inline LV2_URID type()       const { return _type; }
	inline LV2_URID value_type() const { return _value_type; }
	inline uint32_t capacity()   const { return _capacity; }
	inline uint32_t size()       const {
		return is_audio() ? _capacity : sizeof(LV2_Atom) + get<LV2_Atom>()->size;
	}

	void set_type(LV2_URID type, LV2_URID value_type);

	inline bool is_audio() const {
		return _type == _factory.uris().atom_Sound;
	}

	inline bool is_control() const {
		return _type == _factory.uris().atom_Float;
	}

	inline bool is_sequence() const {
		return _type == _factory.uris().atom_Sequence;
	}

	/// Audio or float buffers only
	inline const Sample* samples() const {
		if (is_control()) {
			return (const Sample*)LV2_ATOM_BODY_CONST(get<LV2_Atom_Float>());
		} else if (is_audio()) {
			return (const Sample*)_buf;
		}
		return NULL;
	}

	/// Audio buffers only
	inline Sample* samples() {
		if (is_control()) {
			return (Sample*)LV2_ATOM_BODY(get<LV2_Atom_Float>());
		} else if (is_audio()) {
			return (Sample*)_buf;
		}
		return NULL;
	}

	/// Audio buffers only
	inline SampleCount nframes() const {
		if (is_control()) {
			return 1;
		} else if (is_audio()) {
			return (_capacity / sizeof(Sample));
		}
		return 0;
	}

	/// Numeric buffers only
	inline Sample value_at(SampleCount offset) const {
		if (is_audio() || is_control()) {
			return samples()[offset];
		} else if (_value_buffer) {
			return ((LV2_Atom_Float*)value())->body;
		}
		return 0.0f;
	}

	inline void set_block(const Sample      val,
	                      const SampleCount start,
	                      const SampleCount end)
	{
		assert(is_audio() || is_control());
		assert(end <= nframes());
		// Note: Do not change this without ensuring GCC can still vectorize it
		Sample* const buf = samples() + start;
		for (SampleCount i = 0; i < (end - start); ++i) {
			buf[i] = val;
		}
	}

	inline void add_block(const Sample      val,
	                      const SampleCount start,
	                      const SampleCount end)
	{
		assert(is_audio() || is_control());
		assert(end <= nframes());
		// Note: Do not change this without ensuring GCC can still vectorize it
		Sample* const buf = samples() + start;
		for (SampleCount i = 0; i < (end - start); ++i) {
			buf[i] += val;
		}
	}

	inline void write_block(const Sample      val,
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
	float peak(const RunContext& context) const;

	/// Sequence buffers only
	void prepare_output_write(RunContext& context);

	/// Sequence buffers only
	bool append_event(int64_t        frames,
	                  uint32_t       size,
	                  uint32_t       type,
	                  const uint8_t* data);

	/// Sequence buffers only
	bool append_event(int64_t frames, const LV2_Atom* body);

	/// Value buffer for numeric sequences
	BufferRef       value_buffer()       { return _value_buffer; }
	const BufferRef value_buffer() const { return _value_buffer; }

	const LV2_Atom* value() const;
	LV2_Atom*       value();

	/// Return offset of the first value change after `offset`.
	SampleCount next_value_offset(SampleCount offset, SampleCount end) const;

	/// Update value buffer to value as of offset
	void update_value_buffer(SampleCount offset);

	/// Set/add to audio buffer from the Sequence of Float in `src`
	void render_sequence(const RunContext& context, const Buffer* src, bool add);

	void set_capacity(uint32_t capacity) { _capacity = capacity; }

	void set_buffer(void* buf) { assert(_external); _buf = buf; }

	template<typename T> const T* get() const { return reinterpret_cast<const T*>(_buf); }
	template<typename T> T*       get()       { return reinterpret_cast<T*>(_buf); }

	inline void ref() { ++_refs; }

	inline void deref() {
		if ((--_refs) == 0) {
			recycle();
		}
	}

protected:
	BufferFactory& _factory;
	void*          _buf;
	LV2_URID       _type;
	LV2_URID       _value_type;
	uint32_t       _capacity;
	int64_t        _latest_event;

	BufferRef _value_buffer;  ///< Value buffer for numeric sequences

	friend class BufferFactory;
	~Buffer();

private:
	void recycle();

	Buffer*               _next;      ///< Intrusive linked list for BufferFactory
	std::atomic<unsigned> _refs;      ///< Intrusive reference count
	bool                  _external;  ///< Buffer is externally allocated
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_BUFFER_HPP
