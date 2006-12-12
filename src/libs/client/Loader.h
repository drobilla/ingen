/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include <glibmm/ustring.h>
#include "raul/SharedPtr.h"
#include "raul/Path.h"
#include "Namespaces.h"
#include "ObjectModel.h"

namespace Ingen {
namespace Client {

class ModelEngineInterface;


/** Loads objects (patches, nodes, etc) into the engine from RDF.
 */
class Loader {
public:
	Loader(SharedPtr<ModelEngineInterface> engine, SharedPtr<Namespaces> = SharedPtr<Namespaces>());

	bool load(const Glib::ustring& filename,
	          const Path&          parent,
	          Glib::ustring        patch_uri = "",
	          MetadataMap          initial_data = MetadataMap());

private:
	//string                          _patch_search_path;
	SharedPtr<ModelEngineInterface> _engine;
	SharedPtr<Namespaces>           _namespaces;
};


} // namespace Client
} // namespace Ingen

#endif // LOADER_H
