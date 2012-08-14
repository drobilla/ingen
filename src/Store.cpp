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

#include <sstream>
#include <string>

#include "ingen/Store.hpp"
#include "raul/log.hpp"

using namespace std;

namespace Ingen {

void
Store::add(GraphObject* o)
{
	if (find(o->path()) != end()) {
		Raul::error << "[Store] Attempt to add duplicate object "
		            << o->path() << endl;
		return;
	}

	insert(make_pair(o->path(), o));

	for (uint32_t i = 0; i < o->num_ports(); ++i) {
		add(o->port(i));
	}
}

/*
  TODO: These methods are currently O(n_children) but should logarithmic.  The
  std::map methods do not allow passing a comparator, but std::upper_bound
  does.  This should be achievable by making a rooted comparator that is a
  normal ordering except compares a special sentinel value as the greatest
  element that is a child of the parent.  Searching for this sentinel should
  then find the end of the descendants in logarithmic time.
*/

Store::iterator
Store::find_descendants_end(const iterator parent)
{
	iterator descendants_end = parent;
	++descendants_end;
	while (descendants_end != end() &&
	       descendants_end->first.is_child_of(parent->first)) {
		++descendants_end;
	}

	return descendants_end;
}

Store::const_iterator
Store::find_descendants_end(const const_iterator parent) const
{
	const_iterator descendants_end = parent;
	++descendants_end;
	while (descendants_end != end() &&
	       descendants_end->first.is_child_of(parent->first)) {
		++descendants_end;
	}

	return descendants_end;
}

Store::const_range
Store::children_range(SharedPtr<const GraphObject> o) const
{
	const const_iterator parent = find(o->path());
	if (parent != end()) {
		const_iterator first_child = parent;
		++first_child;
		return std::make_pair(first_child, find_descendants_end(parent));
	}
	return make_pair(end(), end());
}

void
Store::remove(const iterator top, Objects& removed)
{
	if (top != end()) {
		const iterator descendants_end = find_descendants_end(top);
		removed.insert(top, descendants_end);
		erase(top, descendants_end);
	}
}

void
Store::rename(const iterator top, const Raul::Path& new_path)
{
	const Raul::Path old_path = top->first;

	// Remove the object and all its descendants
	Objects removed;
	remove(top, removed);

	// Rename all the removed objects
	for (Objects::const_iterator i = removed.begin(); i != removed.end(); ++i) {
		const Raul::Path path = (i->first == old_path)
			? new_path
			: new_path.child(
				Raul::Path(i->first.substr(old_path.base().length() - 1)));

		i->second->set_path(path);
		assert(find(path) == end());  // Shouldn't be dropping objects!
		insert(make_pair(path, i->second));
	}
}

unsigned
Store::child_name_offset(const Raul::Path&   parent,
                         const Raul::Symbol& symbol,
                         bool                allow_zero)
{
	unsigned offset = 0;

	while (true) {
		std::stringstream ss;
		ss << symbol;
		if (offset > 0) {
			ss << "_" << offset;
		}
		if (find(parent.child(Raul::Symbol(ss.str()))) == end() &&
		    (allow_zero || offset > 0)) {
			break;
		} else if (offset == 0) {
			offset = 2;
		} else {
			++offset;
		}
	}

	return offset;
}

} // namespace Ingen
