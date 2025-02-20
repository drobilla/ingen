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

#include <ingen/Store.hpp>

#include <ingen/Node.hpp>
#include <raul/Path.hpp>
#include <raul/Symbol.hpp>

#include <cassert>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

namespace ingen {

void
Store::add(Node* o)
{
	if (find(o->path()) != end()) {
		return;
	}

	emplace(o->path(), std::shared_ptr<Node>(o));

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
	auto descendants_end = parent;
	++descendants_end;
	while (descendants_end != end() &&
	       descendants_end->first.is_child_of(parent->first)) {
		++descendants_end;
	}

	return descendants_end;
}

Store::const_range
Store::children_range(const std::shared_ptr<const Node>& o) const
{
	const auto parent = find(o->path());
	if (parent != end()) {
		auto first_child = parent;
		++first_child;
		return std::make_pair(first_child, find_descendants_end(parent));
	}
	return make_pair(end(), end());
}

void
Store::remove(const iterator top, Objects& removed)
{
	if (top != end()) {
		const auto descendants_end = find_descendants_end(top);
		removed.insert(top, descendants_end);
		erase(top, descendants_end);
	}
}

void
Store::rename(const iterator top, const raul::Path& new_path)
{
	const raul::Path old_path = top->first;

	// Remove the object and all its descendants
	Objects removed;
	remove(top, removed);

	// Rename all the removed objects
	for (const auto& r : removed) {
		const auto path = (r.first == old_path)
			? new_path
			: new_path.child(
				raul::Path(r.first.substr(old_path.base().length() - 1)));

		r.second->set_path(path);
		assert(find(path) == end()); // Shouldn't be dropping objects!
		emplace(path, r.second);
	}
}

unsigned
Store::child_name_offset(const raul::Path&   parent,
                         const raul::Symbol& symbol,
                         bool                allow_zero) const
{
	unsigned offset = 0;

	while (true) {
		std::stringstream ss;
		ss << symbol;
		if (offset > 0) {
			ss << "_" << offset;
		}

		if (find(parent.child(raul::Symbol(ss.str()))) == end() &&
		    (allow_zero || offset > 0)) {
			break;
		}

		offset = (offset == 0) ? 2 : (offset + 1);
	}

	return offset;
}

} // namespace ingen
