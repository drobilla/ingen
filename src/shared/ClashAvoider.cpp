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


const Raul::Path
ClashAvoider::map_path(const Raul::Path& in)
{
	//cout << "MAP PATH: " << in << endl;
	
	unsigned offset = 0;
	bool has_offset = false;
	size_t pos = in.find_last_of("_");
	if (pos != string::npos && pos != (in.length()-1)) {
		const std::string trailing = in.substr(in.find_last_of("_")+1);
		has_offset = (sscanf(trailing.c_str(), "%u", &offset) > 0);
	}

	//cout << "HAS OFFSET: " << offset << endl;
		
	// Path without _n suffix
	Path base_path = in;
	if (has_offset)
		base_path = base_path.substr(0, base_path.find_last_of("_"));

	//cout << "\tBASE: " << base_path << endl;

	SymbolMap::iterator m = _symbol_map.find(in);
	if (m != _symbol_map.end()) {
		//cout << " (1) " << m->second << endl;
		return m->second;
	} else {
		typedef std::pair<SymbolMap::iterator, bool> InsertRecord;

		// No clash, use symbol unmodified
		if (!exists(in) && _symbol_map.find(in) == _symbol_map.end()) {
			InsertRecord i = _symbol_map.insert(make_pair(in, in));
			assert(i.second);
			//cout << " (2) " << i.first->second << endl;;
			return i.first->second;
		} else {

			// See if the parent is mapped
			// FIXME: do this the other way around
			Path parent = in.parent();
			do {
				SymbolMap::iterator p = _symbol_map.find(parent);
				if (p != _symbol_map.end()) {
					const Path mapped = p->second.base() + in.substr(parent.base().length());
					InsertRecord i = _symbol_map.insert(make_pair(in, mapped));
					//cout << " (3) " << _prefix.base() + i.first->second.substr(1) << endl;
					return i.first->second;
				}
				parent = parent.parent();
			} while (parent != "/");

			// Append _2 _3 etc until an unused symbol is found
			while (true) {
				Offsets::iterator o = _offsets.find(base_path);
				if (o != _offsets.end()) {
					offset = ++o->second;
				} else {
					string parent_str = _prefix.base() + in.parent().base().substr(1);
					parent_str = parent_str.substr(0, parent_str.find_last_of("/"));
					if (parent_str == "")
						parent_str = "/";
					//cout << "***** PARENT: " << parent_str << endl;
				}

				std::stringstream ss;
				ss << base_path << "_" << offset;
				if (!exists(ss.str())) {
					string str = ss.str();
					InsertRecord i = _symbol_map.insert(make_pair(in, str));
					//cout << "HIT: offset = " << offset << ", str = " << str << endl;
					offset = _store.child_name_offset(in.parent(), base_path.name(), false);
					_offsets.insert(make_pair(base_path, offset));
					//cout << " (4) " << i.first->second << endl;;
					return i.first->second;
				} else {
					//cout << "MISSED OFFSET: " << in << " => " << ss.str() << endl;
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
	bool exists = (_store.find(_prefix.base() + path.substr(1)) != _store.end());
	if (exists)
		return true;

	if (_also_avoid)
		return (_also_avoid->find(path) != _also_avoid->end());
	else
		return false;
}


bool
ClashAvoider::new_object(const GraphObject* object)
{
	return false;
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
                       const std::string& type,
                       uint32_t           index,
                       bool               is_output)
{
	_target.new_port(map_path(path), type, index, is_output);
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


void
ClashAvoider::destroy(const std::string& path)
{
	_target.destroy(map_path(path));
}


} // namespace Shared
} // namespace Ingen
