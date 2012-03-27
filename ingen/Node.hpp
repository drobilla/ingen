/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef INGEN_INTERFACE_NODE_HPP
#define INGEN_INTERFACE_NODE_HPP

#include <stdint.h>

#include "ingen/GraphObject.hpp"

namespace Ingen {

class Port;
class Plugin;

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
	virtual uint32_t      num_ports()          const = 0;
	virtual Port*         port(uint32_t index) const = 0;
	virtual const Plugin* plugin()             const = 0;
};

} // namespace Ingen

#endif // INGEN_INTERFACE_NODE_HPP
