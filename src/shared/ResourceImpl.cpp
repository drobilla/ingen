/* This file is part of Ingen.
 * Copyright (C) 2008-2009 Dave Robillard <http://drobilla.net>
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

#include "raul/Atom.hpp"
#include "ResourceImpl.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {


const Raul::URI
ResourceImpl::meta_uri(const Raul::URI& base, const Raul::URI& uri)
{
	return string("meta:#") + uri.chop_start("/");
}


void
ResourceImpl::add_property(const Raul::URI& uri, const Raul::Atom& value)
{
	// Ignore duplicate statements
	typedef Resource::Properties::const_iterator iterator;
	const std::pair<iterator,iterator> range = _properties.equal_range(uri);
	for (iterator i = range.first; i != range.second && i != _properties.end(); ++i)
		if (i->second == value)
			return;

	_properties.insert(make_pair(uri, value));
	signal_property.emit(uri, value);
}


Raul::Atom&
ResourceImpl::set_property(const Raul::URI& uri, const Raul::Atom& value)
{
	_properties.erase(uri);
	return _properties.insert(make_pair(uri, value))->second;
}


Raul::Atom&
ResourceImpl::set_property(const Raul::URI& uri, const Raul::Atom& value) const
{
	return const_cast<ResourceImpl*>(this)->set_property(uri, value);
}


const Raul::Atom&
ResourceImpl::get_property(const Raul::URI& uri) const
{
	static const Raul::Atom nil;
	Properties::const_iterator i = _properties.find(uri);
	return (i != _properties.end()) ? i->second : nil;
}


bool
ResourceImpl::type(
		const Properties& properties,
		bool& patch,
		bool& node,
		bool& port, bool& is_output, DataType& data_type)
{
	typedef Resource::Properties::const_iterator iterator;
	const std::pair<iterator,iterator> types_range = properties.equal_range("rdf:type");

	patch = node = port = is_output = false;
	data_type = DataType::UNKNOWN;
	for (iterator i = types_range.first; i != types_range.second; ++i) {
		const Atom& atom = i->second;
		if (atom.type() == Atom::URI) {
			cerr << "TYPE: " << atom.get_uri() << endl;
			if (!strncmp(atom.get_uri(), "ingen:", 6)) {
				const char* suffix = atom.get_uri() + 6;
				if (!strcmp(suffix, "Patch")) {
					patch = true;
				} else if (!strcmp(suffix, "Node")) {
					node = true;
				}
			} else if (!strncmp(atom.get_uri(), "lv2:", 4)) {
				const char* suffix = atom.get_uri() + 4;
				port = true;
				if (!strcmp(suffix, "InputPort")) {
					is_output = false;
					port = true;
				} else if (!strcmp(suffix, "OutputPort")) {
					is_output = true;
					port = true;
				} else if (!strcmp(suffix, "AudioPort")) {
					data_type = DataType::AUDIO;
					port = true;
				} else if (!strcmp(suffix, "ControlPort")) {
					data_type = DataType::CONTROL;
					port = true;
				}
			} else if (!strcmp(atom.get_uri(), "lv2ev:EventPort")) {
				data_type = DataType::EVENT;
				port = true;
			}
		}
	}

	if (patch && node && !port) { // => patch
		node = false;
		return true;
	} else if (port && (patch || node)) { // nonsense
		port = false;
		return false;
	} else if (patch || node || port) { // recognized type
		return true;
	} else { // unknown
		return false;
	}
}


void
ResourceImpl::set_properties(const Properties& p)
{
	typedef Resource::Properties::const_iterator iterator;
	for (iterator i = p.begin(); i != p.end(); ++i)
		_properties.erase(i->first);
	for (iterator i = p.begin(); i != p.end(); ++i)
		set_property(i->first, i->second);
}


void
ResourceImpl::add_properties(const Properties& p)
{
	typedef Resource::Properties::const_iterator iterator;
	for (iterator i = p.begin(); i != p.end(); ++i)
		_properties.erase(i->first);
	for (iterator i = p.begin(); i != p.end(); ++i)
		add_property(i->first, i->second);
}


} // namespace Shared
} // namespace Ingen
