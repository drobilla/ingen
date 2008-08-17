/* This file is part of Ingen.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
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

#include "ClashAvoider.hpp"
#include "Store.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {


const Raul::Path&
ClashAvoider::map_path(const Raul::Path& in)
{
	SymbolMap::iterator m = _symbol_map.find(in);
	if (m != _symbol_map.end()) {
		return m->second;
	} else {
		typedef std::pair<SymbolMap::iterator, bool> InsertRecord;
		Store::iterator s = _store.find(in);

		// No clash, use symbol unmodified
		if (s == _store.end()) {
			InsertRecord i = _symbol_map.insert(make_pair(in, in));
			return i.first->second;
		} else {

			// See if the parent is mapped
			Path parent = in.parent();
			do {
				SymbolMap::iterator p = _symbol_map.find(parent);
				if (p != _symbol_map.end()) {
					const Path mapped = p->second.base() + in.substr(parent.base().length());
					InsertRecord i = _symbol_map.insert(make_pair(in, mapped));
					return i.first->second;
				}
			} while (parent != "/");

			// Append _2 _3 etc until an unused symbol is found
			string base_name = in.name();
			unsigned offset = 0;
			if (sscanf(base_name.c_str(), "%*s_%u", &offset) == 1)
				base_name = base_name.substr(0, base_name.find_last_of("_"));
			else
				offset = _store.child_name_offset(in.parent(), in.name());

			assert(offset != 0); // shouldn't have been a clash, then...
			std::stringstream ss;
			ss << in << "_" << offset;
			InsertRecord i = _symbol_map.insert(make_pair(in, ss.str()));
			assert(_store.find(i.first->second) == _store.end());
			return i.first->second;
		}
	}
}


void
ClashAvoider::new_patch(const std::string& path,
                        uint32_t           poly)
{
	_target.new_patch(map_path(path), poly);
}


void
ClashAvoider::new_node(const std::string& path,
                       const std::string& plugin_uri)
{
	_target.new_node(map_path(path), plugin_uri);
}


void
ClashAvoider::new_port(const std::string& path,
                       uint32_t           index,
                       const std::string& data_type,
                       bool               is_output)
{
	_target.new_port(map_path(path), index, data_type, is_output);
}


void
ClashAvoider::connect(const std::string& src_port_path,
                      const std::string& dst_port_path)
{
	_target.connect(map_path(src_port_path), map_path(dst_port_path));
}


void
ClashAvoider::disconnect(const std::string& src_port_path,
                         const std::string& dst_port_path)
{
	_target.disconnect(map_path(src_port_path), map_path(dst_port_path));
}


void
ClashAvoider::set_variable(const std::string& subject_path,
                           const std::string& predicate,
                           const Raul::Atom&  value)
{
	_target.set_variable(map_path(subject_path), predicate, value);
}


void
ClashAvoider::set_property(const std::string& subject_path,
                           const std::string& predicate,
                           const Raul::Atom&  value)
{
	_target.set_property(map_path(subject_path), predicate, value);
}


void
ClashAvoider::set_port_value(const std::string& port_path,
                             const Raul::Atom&  value)
{
	_target.set_port_value(map_path(port_path), value);
}


void
ClashAvoider::set_voice_value(const std::string& port_path,
                              uint32_t           voice,
                              const Raul::Atom&  value)
{
	_target.set_voice_value(map_path(port_path), voice, value);
}


} // namespace Shared
} // namespace Ingen
