/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef INGEN_MODULE_MODULE_HPP
#define INGEN_MODULE_MODULE_HPP

#include <string>
#include <glibmm/module.h>
#include "raul/SharedPtr.hpp"

namespace Ingen {
namespace Shared {

class World;

/** A dynamically loaded Ingen module.
 *
 * All components of Ingen reside in one of these.
 */
struct Module {
	virtual ~Module() {}
	virtual void load(Ingen::Shared::World* world) = 0;

	SharedPtr<Glib::Module> library;
};


} // namespace Shared
} // namespace Ingen

#endif // INGEN_MODULE_MODULE_HPP
