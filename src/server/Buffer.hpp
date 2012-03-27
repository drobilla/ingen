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
#include <cstddef>

#include <boost/intrusive_ptr.hpp>
#include <boost/utility.hpp>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
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

class Buffer : public boost::noncopyable, public Raul::Deletable
{
public:
	Buffer(BufferFactory& bufs, LV2_URID type, uint32_t capacity);

	virtual void clear();
	virtual void resize(uint32_t size);
	virtual void copy(Context& context, const Buffer* src);
	virtual void prepare_read(Context& context) {}
	virtual void prepare_write(Context& context);

	void*       port_data(PortType port_type, SampleCount offset);
	const void* port_data(PortType port_type, SampleCount offset) const;

	LV2_URID type()     const { return _type; }
	uint32_t capacity() const { return _capacity; }

	/// Sequence buffers only
	bool append_event(int64_t        frames,
	                  uint32_t       size,
	                  uint32_t       type,
	                  const uint8_t* data);

	LV2_Atom*       atom()       { return _atom; }
	const LV2_Atom* atom() const { return _atom; }

	inline void ref() { ++_refs; }

	inline void deref() {
		if ((--_refs) == 0)
			_factory.recycle(this);
	}

protected:
	BufferFactory& _factory;
	LV2_Atom*      _atom;
	LV2_URID       _type;
	uint32_t       _capacity;

	friend class BufferFactory;
	virtual ~Buffer();

private:
	Buffer*         _next; ///< Intrusive linked list for BufferFactory
	Raul::AtomicInt _refs; ///< Intrusive reference count for intrusive_ptr
};

} // namespace Server
} // namespace Ingen

namespace boost {
inline void intrusive_ptr_add_ref(Ingen::Server::Buffer* b) { b->ref(); }
inline void intrusive_ptr_release(Ingen::Server::Buffer* b) { b->deref(); }
}

#endif // INGEN_ENGINE_BUFFER_HPP
