/* This file is part of Ingen.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
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

#ifndef COMMON_STORE_H
#define COMMON_STORE_H

#include <raul/PathTable.hpp>
#include "interface/GraphObject.hpp"

using Raul::PathTable;
using Raul::Path;

namespace Ingen {
namespace Shared {


class Store {
public:
	typedef Raul::PathTable< SharedPtr<Shared::GraphObject> > Objects;
	virtual const Objects& objects() const = 0;
	
	virtual Objects::iterator find(const Path& path) = 0;
	virtual void add(GraphObject* o) = 0;

	virtual Objects::const_iterator children_begin(SharedPtr<Shared::GraphObject> o) const = 0;
	virtual Objects::const_iterator children_end(SharedPtr<Shared::GraphObject> o) const = 0;
};


} // namespace Shared
} // namespace Ingen

#endif // COMMON_STORE_H
