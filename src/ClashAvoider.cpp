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

#include <cstdio>
#include <sstream>
#include <string>
#include <utility>

#include "ingen/ClashAvoider.hpp"
#include "ingen/Store.hpp"

using namespace std;

namespace Ingen {

const Raul::URI
ClashAvoider::map_uri(const Raul::URI& in)
{
	if (Node::uri_is_path(in)) {
		return Node::path_to_uri(map_path(Node::uri_to_path(in)));
	} else {
		return in;
	}
}

const Raul::Path
ClashAvoider::map_path(const Raul::Path& in)
{
	unsigned offset = 0;
	bool has_offset = false;
	const size_t pos = in.find_last_of('_');
	if (pos != string::npos && pos != (in.length()-1)) {
		const std::string trailing = in.substr(pos + 1);
		has_offset = (sscanf(trailing.c_str(), "%u", &offset) > 0);
	}

	// Path without _n suffix
	std::string base_path_str = in;
	if (has_offset) {
		base_path_str = base_path_str.substr(0, base_path_str.find_last_of('_'));
	}

	Raul::Path base_path(base_path_str);

	SymbolMap::iterator m = _symbol_map.find(in);
	if (m != _symbol_map.end()) {
		return m->second;
	} else {
		typedef std::pair<SymbolMap::iterator, bool> InsertRecord;

		// See if parent is mapped
		Raul::Path parent = in.parent();
		do {
			SymbolMap::iterator p = _symbol_map.find(parent);
			if (p != _symbol_map.end()) {
				const Raul::Path mapped = Raul::Path(
					p->second.base() + in.substr(parent.base().length()));
				InsertRecord i = _symbol_map.insert(make_pair(in, mapped));
				return i.first->second;
			}
			parent = parent.parent();
		} while (!parent.is_root());

		// No clash, use symbol unmodified
		if (!exists(in) && _symbol_map.find(in) == _symbol_map.end()) {
			InsertRecord i = _symbol_map.insert(make_pair(in, in));
			assert(i.second);
			return i.first->second;

		// Append _2 _3 etc until an unused symbol is found
		} else {
			while (true) {
				Offsets::iterator o = _offsets.find(base_path);
				if (o != _offsets.end()) {
					offset = ++o->second;
				} else {
					string parent_str = in.parent().base();
					parent_str = parent_str.substr(0, parent_str.find_last_of("/"));
					if (parent_str.empty())
						parent_str = "/";
				}

				if (offset == 0)
					offset = 2;

				std::stringstream ss;
				ss << base_path << "_" << offset;
				if (!exists(Raul::Path(ss.str()))) {
					std::string name = base_path.symbol();
					if (name == "")
						name = "_";
					Raul::Symbol sym(name);
					string str = ss.str();
					InsertRecord i = _symbol_map.insert(
						make_pair(in, Raul::Path(str)));
					offset = _store.child_name_offset(in.parent(), sym, false);
					_offsets.insert(make_pair(base_path, offset));
					return i.first->second;
				} else {
					if (o != _offsets.end())
						offset = ++o->second;
					else
						++offset;
				}
			}
		}
	}
}

bool
ClashAvoider::exists(const Raul::Path& path) const
{
	bool exists = (_store.find(path) != _store.end());
	if (exists)
		return true;

	if (_also_avoid)
		return (_also_avoid->find(path) != _also_avoid->end());
	else
		return false;
}

void
ClashAvoider::put(const Raul::URI&            path,
                  const Resource::Properties& properties,
                  Resource::Graph             ctx)
{
	_target.put(map_uri(path), properties, ctx);
}

void
ClashAvoider::delta(const Raul::URI&            path,
                    const Resource::Properties& remove,
                    const Resource::Properties& add)
{
	_target.delta(map_uri(path), remove, add);
}

void
ClashAvoider::move(const Raul::Path& old_path,
                   const Raul::Path& new_path)
{
	_target.move(map_path(old_path), map_path(new_path));
}

void
ClashAvoider::connect(const Raul::Path& tail,
                      const Raul::Path& head)
{
	_target.connect(map_path(tail), map_path(head));
}

void
ClashAvoider::disconnect(const Raul::Path& tail,
                         const Raul::Path& head)
{
	_target.disconnect(map_path(tail), map_path(head));
}

void
ClashAvoider::disconnect_all(const Raul::Path& graph,
                             const Raul::Path& path)
{
	_target.disconnect_all(map_path(graph), map_path(path));
}

void
ClashAvoider::set_property(const Raul::URI&  subject,
                           const Raul::URI&  predicate,
                           const Raul::Atom& value)
{
	_target.set_property(map_uri(subject), predicate, value);
}

void
ClashAvoider::del(const Raul::URI& uri)
{
	_target.del(map_uri(uri));
}

} // namespace Ingen
