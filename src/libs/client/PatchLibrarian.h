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

#ifndef PATCHLIBRARIAN_H
#define PATCHLIBRARIAN_H

#include <map>
#include <utility>
#include <string>
#include <libxml/tree.h>
#include <cassert>
#include "util/CountedPtr.h"

using std::string;

namespace Ingen {
namespace Client {

class PatchModel;
class NodeModel;
class ConnectionModel;
class PresetModel;
class ModelEngineInterface;

	
/** Handles all patch saving and loading.
 *
 * \ingroup IngenClient
 */
class PatchLibrarian
{
public:
	// FIXME: return booleans and set an errstr that can be checked or something?
	
	PatchLibrarian(CountedPtr<ModelEngineInterface> engine)
	: _patch_search_path("."), _engine(engine)
	{
		assert(_engine);
	}

	void          path(const string& path) { _patch_search_path = path; }
	const string& path()                   { return _patch_search_path; }
	
	string find_file(const string& filename, const string& additional_path = "");
	
	void save_patch(CountedPtr<PatchModel> patch_model, const string& filename, bool recursive);
	string load_patch(CountedPtr<PatchModel> pm, bool wait = true, bool existing = false);

private:
	string translate_load_path(const string& path);

	string                           _patch_search_path;
	CountedPtr<ModelEngineInterface> _engine;

	/// Translations of paths from the loading file to actual paths (for deprecated patches)
	std::map<string, string> _load_path_translations;

	CountedPtr<NodeModel> parse_node(const CountedPtr<const PatchModel> parent, xmlDocPtr doc, const xmlNodePtr cur);
	ConnectionModel* parse_connection(const CountedPtr<const PatchModel> parent, xmlDocPtr doc, const xmlNodePtr cur);
	PresetModel*     parse_preset(const CountedPtr<const PatchModel> parent, xmlDocPtr doc, const xmlNodePtr cur);
	void             load_subpatch(const CountedPtr<PatchModel> parent, xmlDocPtr doc, const xmlNodePtr cur);
};


} // namespace Client
} // namespace Ingen

#endif // PATCHLIBRARIAN_H
