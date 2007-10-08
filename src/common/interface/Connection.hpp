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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <raul/Path.hpp>

namespace Ingen {
namespace Shared {


/** A connection between two ports.
 *
 * \ingroup interface
 */
class Connection
{
public:
	virtual const Raul::Path src_port_path() const = 0;
	virtual const Raul::Path dst_port_path() const = 0;
};


} // namespace Shared
} // namespace Ingen

#endif // CONNECTION_H
