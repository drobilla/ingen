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

#include CONFIG_H_PATH
#include "module/World.hpp"
#include "serialisation.hpp"
#include "Loader.hpp"
#include "Serialiser.hpp"

namespace Ingen {
namespace Serialisation {


Ingen::Serialisation::Loader*
new_loader()
{
	return new Loader();
}

	
Ingen::Serialisation::Serialiser*
new_serialiser(Ingen::Shared::World* world)
{
	assert(world->rdf_world);
	return new Serialiser(*world->rdf_world);
}



} // namespace Serialisation
} // namespace Ingen

