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
#include "raul/PathTable.hpp"
#include "raul/TableImpl.hpp"
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
