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

#ifndef LOADER_H
#define LOADER_H

#include <string>
#include <glibmm/ustring.h>
#include <boost/optional.hpp>
#include <raul/SharedPtr.hpp>
#include <raul/Path.hpp>
#include <raul/Table.hpp>
#include "interface/GraphObject.hpp"

namespace Raul { namespace RDF { class World; } }
namespace Ingen { namespace Shared { class EngineInterface; } }

using namespace Ingen::Shared;

namespace Ingen {
namespace Serialisation {


class Loader {
public:
	virtual ~Loader() {}
	
	virtual bool
	load(SharedPtr<Ingen::Shared::EngineInterface> engine,
	     Raul::RDF::World*                         world,
	     const Glib::ustring&                      uri,
	     boost::optional<Raul::Path>               parent,
	     std::string                               patch_name,
	     Glib::ustring                             patch_uri = "",
	     GraphObject::MetadataMap                  data = GraphObject::MetadataMap());
};


} // namespace Serialisation
} // namespace Ingen

#endif // LOADER_H
