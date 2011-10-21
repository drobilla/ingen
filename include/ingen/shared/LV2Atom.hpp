/* This file is part of Ingen.
 * Copyright 2009-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_SHARED_LV2ATOM_HPP
#define INGEN_SHARED_LV2ATOM_HPP

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

namespace Raul { class Atom; }

namespace Ingen {
namespace Shared {

class URIs;

namespace LV2Atom {

bool to_atom(const URIs&     uris,
             const LV2_Atom* object,
             Raul::Atom&     atom);

bool from_atom(const URIs&       uris,
               const Raul::Atom& atom,
               LV2_Atom*         object);

} // namespace LV2Atom

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_LV2ATOM_HPP
