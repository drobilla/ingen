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

#ifndef BUILDER_H
#define BUILDER_H

#include <raul/SharedPtr.hpp>

namespace Raul { class Path; }

namespace Ingen {
namespace Shared {

class GraphObject;
class CommonInterface;


/** Wrapper for CommonInterface to create existing objects/models.
 *
 * \ingroup interface
 */
class Builder
{
public:
	Builder(CommonInterface& interface);
	virtual ~Builder() {}

	void build(const Raul::Path&            prefix,
	           SharedPtr<const GraphObject> object);

private:
	void build_object(const Raul::Path&            prefix,
	                  SharedPtr<const GraphObject> object);

	CommonInterface& _interface;
};


} // namespace Shared
} // namespace Ingen

#endif // BUILDER_H

