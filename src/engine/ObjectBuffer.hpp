/* This file is part of Ingen.
 * Copyright (C) 2009 Dave Robillard <http://drobilla.net>
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

#ifndef OBJECTBUFFER_H
#define OBJECTBUFFER_H

#include "raul/Atom.hpp"
#include "object.lv2/object.h"
#include "interface/DataType.hpp"
#include "Buffer.hpp"

namespace Ingen {

class Context;

class ObjectBuffer : public Buffer {
public:
	ObjectBuffer(size_t capacity);

	void clear();

	void*       port_data(Shared::DataType port_type);
	const void* port_data(Shared::DataType port_type) const;

	void copy(Context& context, const Buffer* src);
	void mix(Context& context, const Buffer* src) {}

	void resize(size_t size);

	LV2_Object*       object()       { return _buf; }
	const LV2_Object* object() const { return _buf; }

private:
	LV2_Object* _buf; ///< Contents
};


} // namespace Ingen

#endif // OBJECTBUFFER_H
