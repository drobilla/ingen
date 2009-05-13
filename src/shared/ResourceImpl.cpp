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

#include "raul/Atom.hpp"
#include "ResourceImpl.hpp"

namespace Ingen {
namespace Shared {


void
ResourceImpl::set_property(const Raul::URI& uri, const Raul::Atom& value)
{
	_properties[uri] = value;
}


const Raul::Atom&
ResourceImpl::get_property(const Raul::URI& uri) const
{
	static const Raul::Atom nil;
	Properties::const_iterator i = _properties.find(uri);
	return (i != _properties.end()) ? i->second : nil;
}


} // namespace Shared
} // namespace Ingen
