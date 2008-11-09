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

#ifndef RESOURCEIMPL_HPP
#define RESOURCEIMPL_HPP

#include <map>
#include "interface/Resource.hpp"

namespace Ingen {
namespace Shared {


class ResourceImpl : virtual public Resource
{
public:
	typedef std::map<std::string, Raul::Atom> Properties;

	ResourceImpl(const std::string& uri) : _uri(uri) {}

	virtual const std::string uri() const { return _uri; }

	const Properties& properties() const { return _properties; }
	Properties&       properties()       { return _properties; }

	void set_property(const std::string& uri, const Raul::Atom& value) {
		_properties[uri] = value;
	}

	const Raul::Atom& get_property(const std::string& uri) const {
		static const Raul::Atom nil;
		Properties::const_iterator i = _properties.find(uri);
		if (i == _properties.end())
			return nil;
		else
			return i->second;
	}

private:
	std::string _uri;
	Properties  _properties;
};


} // namespace Shared
} // namespace Ingen

#endif // RESOURCEIMPL_HPP

