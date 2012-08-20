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

#ifndef INGEN_ENGINE_BUFFER_HPP
#define INGEN_ENGINE_BUFFER_HPP

#include <cassert>

#include <boost/utility.hpp>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "raul/AtomicInt.hpp"
#include "raul/Deletable.hpp"
#include "raul/SharedPtr.hpp"

#include "BufferFactory.hpp"
#include "PortType.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {

class Context;
class Engine;
class BufferFactory;

class Buffer : public boost::noncopyable
{
public:
	Buffer(BufferFactory& bufs, LV2_URID type, uint32_t capacity);

	void clear();
	void resize(uint32_t size);
	void copy(const Context& context, const Buffer* src);
	void prepare_write(Context& context);

	void*       port_data(PortType port_type);
	const void* port_data(PortType port_type) const;

	inline LV2_URID type()     const { return _type; }
	inline uint32_t capacity() const { return _capacity; }

	inline bool is_audio() const {
		return _type == _factory.uris().atom_Sound;
	}

	inline bool is_control() const {
		return _type == _factory.uris().atom_Float;
	}

	inline bool is_sequence() const {
		return _type == _factory.uris().atom_Sequence;
	}

	/// Audio buffers only
	inline const Sample* samples() const {
		if (is_control()) {
			return (const Sample*)LV2_ATOM_BODY_CONST(atom());
		} else if (is_audio()) {
			return (const Sample*)LV2_ATOM_CONTENTS_CONST(LV2_Atom_Vector, atom());
		}
		return NULL;
	}

	/// Audio buffers only
	inline Sample* samples() {
		if (is_control()) {
			return (Sample*)LV2_ATOM_BODY(atom());
		} else if (is_audio()) {
			return (Sample*)LV2_ATOM_CONTENTS(LV2_Atom_Vector, atom());
		}
		return NULL;
	}

	/// Audio buffers only
	inline SampleCount nframes() const {
		if (is_control()) {
			return 1;
		} else if (is_audio()) {
			return (_capacity - sizeof(LV2_Atom_Vector)) / sizeof(Sample);
		}
		return 0;
	}

	/// Audio buffers only
	inline Sample value_at(SampleCount offset) const {
		if (is_audio() || is_control()) {
			return samples()[offset];
		}
		return 0.0f;
	}

	inline void set_block(Sample            val,
	                      const SampleCount start,
	                      const SampleCount end)
	{
		assert(end <= nframes());
		// Note: Do not change this without ensuring GCC can still vectorize it
		Sample* const buf = samples() + start;
		for (SampleCount i = 0; i < (end - start); ++i) {
			buf[i] = val;
		}
	}

	/// Audio buffers only
	float peak(const Context& context) const;

	/// Sequence buffers only
	void prepare_output_write(Context& context);

	/// Sequence buffers only
	bool append_event(int64_t        frames,
	                  uint32_t       size,
	                  uint32_t       type,
	                  const uint8_t* data);

	LV2_Atom*       atom()       { return _atom; }
	const LV2_Atom* atom() const { return _atom; }

	void set_capacity(uint32_t capacity) { _capacity = capacity; }

	inline void ref() { ++_refs; }

	inline void deref() {
		if ((--_refs) == 0) {
			recycle();
		}
	}

protected:
	BufferFactory& _factory;
	LV2_Atom*      _atom;
	LV2_URID       _type;
	uint32_t       _capacity;

	friend class BufferFactory;
	~Buffer();

private:
	void recycle();

	Buffer*         _next; ///< Intrusive linked list for BufferFactory
	Raul::AtomicInt _refs; ///< Intrusive reference count
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_BUFFER_HPP
