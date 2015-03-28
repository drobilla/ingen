/*
  This file is part of Ingen.
  Copyright 2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_RDF_HPP
#define INGEN_GUI_RDF_HPP

#include <map>
#include <set>

#include <glibmm/ustring.h>

#include "ingen/types.hpp"
#include "raul/URI.hpp"

namespace Ingen {

class World;

namespace Client { class ObjectModel; }

namespace GUI {

namespace RDFS {

/** Set of URIs. */
typedef std::set<Raul::URI> URISet;

/** Map of object labels, keyed by object URI. */
typedef std::map<Raul::URI, Glib::ustring> Objects;

/** Return the label of `node`. */
Glib::ustring
label(World* world, const LilvNode* node);

/** Return the comment of `node`. */
Glib::ustring
comment(World* world, const LilvNode* node);

/** Set `types` to its super/sub class closure.
 * @param super If true, find all superclasses, otherwise all subclasses
 */
void classes(World* world, URISet& types, bool super);

/** Get all instances of any class in `types`. */
Objects
instances(World* world, const URISet& types);

/** Get all the types which `model` is an instance of. */
URISet types(World* world, SPtr<const Client::ObjectModel> model);

/** Get all the properties with domains appropriate for `model`. */
URISet properties(World* world, SPtr<const Client::ObjectModel> model);

} // namespace RDFS
} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_RDF_HPP
