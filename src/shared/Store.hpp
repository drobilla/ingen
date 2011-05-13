/* This file is part of Ingen.
 * Copyright 2008-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_SHARED_STORE_HPP
#define INGEN_SHARED_STORE_HPP

#include <string>
#include <glibmm/thread.h>
#include "raul/PathTable.hpp"
#include "ingen/GraphObject.hpp"

namespace Ingen {
namespace Shared {

class Store : public Raul::PathTable< SharedPtr<GraphObject> > {
public:
	virtual ~Store() {}

	virtual void add(GraphObject* o);

	typedef Raul::Table< Raul::Path, SharedPtr<GraphObject> > Objects;

	const_iterator children_begin(SharedPtr<const GraphObject> o) const;
	const_iterator children_end(SharedPtr<const GraphObject> o) const;

	SharedPtr<GraphObject> find_child(SharedPtr<const GraphObject> parent,
	                                  const std::string&           child_name) const;

	unsigned child_name_offset(const Raul::Path&   parent,
	                           const Raul::Symbol& symbol,
	                           bool                allow_zero=true);

	Glib::RWLock& lock() { return _lock; }

private:
	Glib::RWLock _lock;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_STORE_HPP
