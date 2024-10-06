/*
  This file is part of Ingen.
  Copyright 2007-2018 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/ClashAvoider.hpp"
#include "ingen/Store.hpp"
#include "ingen/URI.hpp"
#include "ingen/paths.hpp"
#include "raul/Path.hpp"
#include "raul/Symbol.hpp"

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

namespace ingen {

ClashAvoider::ClashAvoider(const Store& store)
	: _store(store)
{}

URI
ClashAvoider::map_uri(const URI& in)
{
	if (uri_is_path(in)) {
		return path_to_uri(map_path(uri_to_path(in)));
	}

	return in;
}

raul::Path
ClashAvoider::map_path(const raul::Path& in)
{
	unsigned offset = 0;
	bool has_offset = false;
	const size_t pos = in.find_last_of('_');
	if (pos != std::string::npos && pos != (in.length()-1)) {
		const std::string trailing = in.substr(pos + 1);
		char*             end      = nullptr;
		strtoul(trailing.c_str(), &end, 10);
		has_offset = (*end == '\0');
	}

	// Path without _n suffix
	std::string base_path_str = in;
	if (has_offset) {
		base_path_str.resize(base_path_str.find_last_of('_'));
	}

	raul::Path base_path(base_path_str);

	auto m = _symbol_map.find(in);
	if (m != _symbol_map.end()) {
		return m->second;
	}

	// See if parent is mapped
	raul::Path parent = in.parent();
	do {
		auto p = _symbol_map.find(parent);
		if (p != _symbol_map.end()) {
			const auto mapped = raul::Path{p->second.base() +
			                               in.substr(parent.base().length())};

			auto i = _symbol_map.emplace(in, mapped);
			return i.first->second;
		}
		parent = parent.parent();
	} while (!parent.is_root());

	if (!exists(in) && _symbol_map.find(in) == _symbol_map.end()) {
		// No clash, use symbol unmodified
		auto i = _symbol_map.emplace(in, in);
		assert(i.second);
		return i.first->second;
	}

	// Append _2 _3 etc until an unused symbol is found
	while (true) {
		auto o = _offsets.find(base_path);
		if (o != _offsets.end()) {
			offset = ++o->second;
		}

		if (offset == 0) {
			offset = 2;
		}

		std::stringstream ss;
		ss << base_path << "_" << offset;
		if (!exists(raul::Path(ss.str()))) {
			std::string name = base_path.symbol();
			if (name.empty()) {
				name = "_";
			}

			const raul::Symbol sym{name};
			const std::string  str{ss.str()};

			auto i = _symbol_map.emplace(in, raul::Path(str));

			offset = _store.child_name_offset(in.parent(), sym, false);
			_offsets.emplace(base_path, offset);
			return i.first->second;
		}

		if (o != _offsets.end()) {
			offset = ++o->second;
		} else {
			++offset;
		}
	}
}

bool
ClashAvoider::exists(const raul::Path& path) const
{
	return _store.find(path) != _store.end();
}

static std::optional<size_t>
numeric_suffix_start(const std::string& str)
{
	if (!isdigit(str[str.length() - 1])) {
		return {};
	}

	size_t i = str.length() - 1;
	while (i > 0 && isdigit(str[i - 1])) {
		--i;
	}

	return i;
}

std::string
ClashAvoider::adjust_name(const raul::Path&  old_path,
                          const raul::Path&  new_path,
                          const std::string& name)
{
	const auto name_suffix_start = numeric_suffix_start(name);
	if (!name_suffix_start) {
		return name; // No numeric suffix, just re-use old label
	}

	const auto name_suffix      = atoi(name.c_str() + *name_suffix_start);
	const auto old_suffix_start = numeric_suffix_start(old_path);
	const auto new_suffix_start = numeric_suffix_start(new_path);
	if (old_suffix_start && new_suffix_start) {
		// Add the offset applied to the symbol to the label suffix
		const auto old_suffix = atoi(old_path.c_str() + *old_suffix_start);
		const auto new_suffix = atoi(new_path.c_str() + *new_suffix_start);
		const auto offset     = new_suffix - old_suffix;
		return (name.substr(0, *name_suffix_start) +
		        std::to_string(name_suffix + offset));
	}

	// Add 1 to previous label suffix
	return (name.substr(0, *name_suffix_start) +
	        std::to_string(name_suffix + 1));
}

} // namespace ingen
