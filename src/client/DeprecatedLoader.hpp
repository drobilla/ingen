/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_CLIENT_DEPRECATEDLOADER_HPP
#define INGEN_CLIENT_DEPRECATEDLOADER_HPP

#include <map>
#include <utility>
#include <string>
#include <cassert>
#include <boost/optional.hpp>
#include <glibmm/ustring.h>
#include <libxml/tree.h>
#include "raul/SharedPtr.hpp"
#include "interface/EngineInterface.hpp"
#include "interface/GraphObject.hpp"
#include "ObjectModel.hpp"

namespace Raul { class Path; }

namespace Ingen {

namespace Shared { class LV2URIMap; }

namespace Client {

class PresetModel; // defined in DeprecatedLoader.cpp


/** Loads deprecated (XML) patch files (from the Om days).
 *
 * \ingroup IngenClient
 */
class DeprecatedLoader
{
public:
	DeprecatedLoader(
			SharedPtr<Shared::LV2URIMap> uris,
			SharedPtr<Shared::EngineInterface> engine)
		: _uris(uris)
		, _engine(engine)
	{
		assert(_engine);
	}

	std::string load_patch(const Glib::ustring&            filename,
	                       bool                            merge,
	                       boost::optional<Raul::Path>     parent_path,
	                       boost::optional<Raul::Symbol>   name,
	                       Shared::GraphObject::Properties initial_data,
	                       bool                            existing = false);

private:
	void add_variable(Shared::GraphObject::Properties& data, std::string key, std::string value);

	std::string nameify_if_invalid(const std::string& name);
	std::string translate_load_path(const std::string& path);

	SharedPtr<Shared::LV2URIMap>       _uris;
	SharedPtr<Shared::EngineInterface> _engine;

	/// Translations of paths from the loading file to actual paths (for deprecated patches)
	std::map<std::string, std::string> _load_path_translations;

	bool load_node(const Raul::Path& parent, xmlDocPtr doc, const xmlNodePtr cur);
	bool load_connection(const Raul::Path& parent, xmlDocPtr doc, const xmlNodePtr cur);
	bool load_subpatch(const std::string& base_filename, const Raul::Path& parent, xmlDocPtr doc, const xmlNodePtr cur);

	SharedPtr<PresetModel> load_preset(const Raul::Path& parent, xmlDocPtr doc, const xmlNodePtr cur);
};


} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_DEPRECATEDLOADER_HPP
