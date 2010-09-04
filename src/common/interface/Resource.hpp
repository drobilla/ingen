/* This file is part of Ingen.
 * Copyright (C) 2008-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_INTERFACE_RESOURCE_HPP
#define INGEN_INTERFACE_RESOURCE_HPP

#include <string>
#include <map>

namespace Raul { class Atom; class URI; }

namespace Ingen {
namespace Shared {


class Resource
{
public:
	virtual ~Resource() {}
	typedef std::multimap<Raul::URI, Raul::Atom> Properties;

	virtual const Raul::URI&  uri()        const = 0;
	virtual const Properties& properties() const = 0;
	virtual Properties&       properties()       = 0;

	virtual Raul::Atom& set_property(const Raul::URI&  uri,
	                                 const Raul::Atom& value) = 0;

	virtual void add_property(const Raul::URI&  uri,
	                          const Raul::Atom& value) = 0;

	virtual const Raul::Atom& get_property(const Raul::URI& uri) const = 0;

	virtual bool has_property(const Raul::URI& uri, const Raul::Atom& value) const = 0;
};


} // namespace Shared
} // namespace Ingen

#endif // INGEN_INTERFACE_RESOURCE_HPP

