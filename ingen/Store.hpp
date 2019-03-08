/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

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

#include "ingen/ingen.h"
#include "ingen/types.hpp"
#include "raul/Deletable.hpp"
#include "raul/Noncopyable.hpp"
#include "raul/Path.hpp"

#include <map>
#include <mutex>
#include <utility>

namespace Raul { class Symbol; }

namespace ingen {

class Node;

/** Store of objects in the graph hierarchy.
 * @ingroup IngenShared
 */
class INGEN_API Store : public Raul::Noncopyable
                      , public Raul::Deletable
                      , public std::map< const Raul::Path, SPtr<Node> > {
public:
	void add(Node* o);

	Node* get(const Raul::Path& path) {
		const iterator i = find(path);
		return (i == end()) ? nullptr : i->second.get();
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
	void rename(iterator top, const Raul::Path& new_path);

	unsigned child_name_offset(const Raul::Path&   parent,
	                           const Raul::Symbol& symbol,
	                           bool                allow_zero=true) const;

	typedef std::recursive_mutex Mutex;

	Mutex& mutex() { return _mutex; }

private:
	Mutex _mutex;
};

} // namespace ingen

#endif // INGEN_STORE_HPP
