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

#include <map>

#undef nil
#include <glibmm/thread.h>

#include "raul/Deletable.hpp"
#include "raul/Noncopyable.hpp"

#include "ingen/Node.hpp"
#include "ingen/types.hpp"

namespace Ingen {

/** Store of objects in the graph hierarchy.
 * @ingroup IngenShared
 */
class Store : public Raul::Noncopyable
            , public Raul::Deletable
            , public std::map< const Raul::Path, SPtr<Node> > {
public:
	void add(Node* o);

	Node* get(const Raul::Path& path) {
		const iterator i = find(path);
		return (i == end()) ? NULL : i->second.get();
	}

	typedef std::pair<const_iterator, const_iterator> const_range;

	typedef std::map< Raul::Path, SPtr<Node> > Objects;

	iterator       find_descendants_end(Store::iterator parent);
	const_iterator find_descendants_end(Store::const_iterator parent) const;

	const_range children_range(SPtr<const Node> o) const;

	/** Remove the object at `top` and all its children from the store.
	 *
	 * @param top Iterator to first (topmost parent) object to remove.
	 *
	 * @param removed Filled with all the removed objects.  Note this may be
	 * many objects since any children will be removed as well.
	 */
	void remove(iterator top, Objects& removed);

	/** Rename (move) the object at `top` to `new_path`.
	 *
	 * Note this invalidates `i`.
	 */
	void rename(iterator i, const Raul::Path& new_path);

	unsigned child_name_offset(const Raul::Path&   parent,
	                           const Raul::Symbol& symbol,
	                           bool                allow_zero=true);

	Glib::RWLock& lock() { return _lock; }

private:
	Glib::RWLock _lock;
};

} // namespace Ingen

#endif // INGEN_STORE_HPP
