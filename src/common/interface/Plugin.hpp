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

#ifndef PLUGIN_H
#define PLUGIN_H

#include <string>

namespace Ingen {
namespace Shared {


class Plugin
{
public:
	enum Type { LV2, LADSPA, DSSI, Internal, Patch };

	virtual Type               type() const = 0;
	virtual const std::string& uri()  const = 0;
	virtual const std::string& name() const = 0;
};


} // namespace Shared
} // namespace Ingen

#endif // PLUGIN_H
