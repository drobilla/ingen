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

#include <ingen/ingen.h>
#include <raul/Deletable.hpp>
#include <raul/Noncopyable.hpp>
#include <raul/Path.hpp>

#include <map>
#include <memory>
#include <mutex>
#include <utility>

namespace raul {
class Symbol;
} // namespace raul

namespace ingen {

class Node;

/** Store of objects in the graph hierarchy.
 * @ingroup IngenShared
 */
class INGEN_API Store : public raul::Noncopyable,
                        public raul::Deletable,
                        public std::map<const raul::Path, std::shared_ptr<Node>>
{
public:
	void add(Node* o);

	Node* get(const raul::Path& path) {
		const auto i = find(path);
		return (i == end()) ? nullptr : i->second.get();
	}

	using const_range = std::pair<const_iterator, const_iterator>;
	using Objects     = std::map<raul::Path, std::shared_ptr<Node>>;
	using Mutex       = std::recursive_mutex;

	iterator       find_descendants_end(Store::iterator parent);
	const_iterator find_descendants_end(Store::const_iterator parent) const;

	const_range children_range(const std::shared_ptr<const Node>& o) const;

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
	void rename(iterator top, const raul::Path& new_path);

	unsigned child_name_offset(const raul::Path&   parent,
	                           const raul::Symbol& symbol,
	                           bool                allow_zero=true) const;

	Mutex& mutex() { return _mutex; }

private:
	Mutex _mutex;
};

} // namespace ingen

#endif // INGEN_STORE_HPP
