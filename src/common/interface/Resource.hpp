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

#ifndef RESOURCE_HPP
#define RESOURCE_HPP

#include <string>
#include <map>
#include "raul/Atom.hpp"

namespace Ingen {
namespace Shared {


class Resource
{
public:
	typedef std::map<std::string, Raul::Atom> Properties;

	virtual const std::string  uri()        const = 0;
	virtual const Properties&  properties() const = 0;
	virtual Properties&        properties()       = 0;

	virtual void set_property(const std::string& uri,
	                          const Raul::Atom&  value) = 0;

	virtual const Raul::Atom& get_property(const std::string& uri) const = 0;
};


} // namespace Shared
} // namespace Ingen

#endif // RESOURCE_HPP

