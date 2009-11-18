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

#ifndef BUFFER_H
#define BUFFER_H

#include <cstddef>
#include <cassert>
#include <boost/utility.hpp>
#include "raul/Deletable.hpp"
#include "raul/SharedPtr.hpp"
#include "types.hpp"
#include "interface/PortType.hpp"

namespace Ingen {

class Context;
class Engine;

class Buffer : public boost::noncopyable, public Raul::Deletable
{
public:
	Buffer(Shared::PortType type, size_t size)
		: _type(type)
		, _size(size)
	{}

	/** Clear contents and reset state */
	virtual void clear() = 0;

	virtual void resize(size_t size) { _size = size; }

	virtual void*       port_data(Shared::PortType port_type) = 0;
	virtual const void* port_data(Shared::PortType port_type) const = 0;

	/** Rewind (ie reset read pointer), but leave contents unchanged */
	virtual void rewind() const {}

	virtual void copy(Context& context, const Buffer* src) = 0;
	virtual void mix(Context& context, const Buffer* src) = 0;

	virtual void prepare_read(Context& context) {}
	virtual void prepare_write(Context& context) {}

	Shared::PortType type() const { return _type; }
	size_t           size() const { return _size; }

protected:
	Shared::PortType _type;
	size_t           _size;
	bool             _local;

	friend class BufferFactory;
	virtual ~Buffer() {}

private:
	Buffer* _next; ///< Intrusive linked list for BufferFactory
};


} // namespace Ingen

#endif // BUFFER_H
