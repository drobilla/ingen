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

#ifndef NODE_H
#define NODE_H

#include <string>
#include <raul/Array.hpp>
#include "GraphObject.hpp"

namespace Raul { template <typename T> class List; class Maid; }

namespace Ingen {
namespace Shared {


/** A Node (or "module") in a Patch (which is also a Node).
 * 
 * A Node is a unit with input/output ports, a process() method, and some other
 * things.
 *
 * Purely virtual (except for the destructor).
 * 
 * \ingroup interface
 */
class Node : public virtual GraphObject
{
public:
	virtual uint32_t num_ports() const = 0;
};


} // namespace Shared
} // namespace Ingen

#endif // NODE_H
