/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_INTERFACE_GRAPHOBJECT_HPP
#define INGEN_INTERFACE_GRAPHOBJECT_HPP

#include "raul/Deletable.hpp"

#include "ingen/Resource.hpp"

namespace Raul { class Atom; class Path; class Symbol; }

namespace Ingen {

/** An object on the audio graph - Patch, Node, Port, etc.
 *
 * Purely virtual (except for the destructor).
 *
 * \ingroup interface
 */
class GraphObject : public Raul::Deletable
                  , public virtual Resource
{
public:
	virtual void set_path(const Raul::Path& path) = 0;

	virtual const Raul::Path&   path()         const = 0;
	virtual const Raul::Symbol& symbol()       const = 0;
	virtual GraphObject*        graph_parent() const = 0;
};

} // namespace Ingen

#endif // INGEN_INTERFACE_GRAPHOBJECT_HPP
