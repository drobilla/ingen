/* This file is part of Ingen.
 * Copyright (C) 2009 Dave Robillard <http://drobilla.net>
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

#ifndef LV2_OBJECT_HPP
#define LV2_OBJECT_HPP

namespace Raul { class Atom; }
typedef struct _LV2_Object LV2_Object;

namespace Ingen {
namespace Shared {

class World;

namespace LV2Object {

	bool to_atom(World* world, const LV2_Object* object, Raul::Atom& atom);
	bool from_atom(World* word,  const Raul::Atom& atom,   LV2_Object* object);

} // namespace LV2Object

} // namespace Shared
} // namespace Ingen

#endif // LV2_OBJECT_HPP
