/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef GRAPHOBJECT_H
#define GRAPHOBJECT_H

#include <string>
#include <map>
#include <raul/Deletable.hpp>
#include <raul/PathTable.hpp>
#include <raul/Symbol.hpp>
#include <raul/Atom.hpp>
#include <raul/SharedPtr.hpp>
#include <raul/WeakPtr.hpp>

using Raul::PathTable;

namespace Ingen {
namespace Shared {


/** An object on the audio graph - Patch, Node, Port, etc.
 *
 * Purely virtual (except for the destructor).
 *
 * \ingroup interface
 */
class GraphObject : public Raul::Deletable
{
public:
	typedef std::map<std::string, Raul::Atom> Variables;
	typedef std::map<std::string, Raul::Atom> Properties;

	typedef PathTable< SharedPtr<GraphObject> >::const_iterator const_iterator;
	
	virtual const Raul::Path   path()       const = 0;
	virtual const Raul::Symbol symbol()     const = 0;
	virtual const Variables&   variables()  const = 0;
	virtual Variables&         variables()        = 0;
	virtual const Properties&  properties() const = 0;
	virtual Properties&        properties()       = 0;
	virtual bool               polyphonic() const = 0;
	
	// FIXME: return WeakPtr, and stupid name
	virtual GraphObject* graph_parent() const = 0;
};


} // namespace Shared
} // namespace Ingen

#endif // GRAPHOBJECT_H
