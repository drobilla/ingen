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

#ifndef INGEN_STORE_HPP
#define INGEN_STORE_HPP

#include <string>

#undef nil
#include <glibmm/thread.h>

#include "raul/PathTable.hpp"

#include "ingen/GraphObject.hpp"

namespace Ingen {

/** Store of objects in the patch hierarchy.
 * @ingroup IngenShared
 */
class Store : public Raul::PathTable< SharedPtr<GraphObject> > {
public:
	virtual ~Store() {}

	virtual void add(GraphObject* o);

	typedef Raul::Table< Raul::Path, SharedPtr<GraphObject> > Objects;

	const_iterator children_begin(SharedPtr<const GraphObject> o) const;
	const_iterator children_end(SharedPtr<const GraphObject> o) const;

	SharedPtr<GraphObject> find_child(SharedPtr<const GraphObject> parent,
	                                  const Raul::Symbol&          symbol) const;

	unsigned child_name_offset(const Raul::Path&   parent,
	                           const Raul::Symbol& symbol,
	                           bool                allow_zero=true);

	Glib::RWLock& lock() { return _lock; }

private:
	Glib::RWLock _lock;
};

} // namespace Ingen

#endif // INGEN_STORE_HPP
