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

#include "GladeFactory.hpp"
#include <fstream>
#include "raul/log.hpp"
#include "ingen-config.h"
#include "shared/runtime_paths.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace GUI {


Glib::ustring GladeFactory::glade_filename = "";

inline static bool
is_readable(const std::string& filename)
{
	std::ifstream fs(filename.c_str());
	const bool fail = fs.fail();
	fs.close();
	return !fail;
}

void
GladeFactory::find_glade_file()
{
	// Try file in bundle (directory where executable resides)
	glade_filename = Shared::bundle_file_path("ingen_gui.glade");
	if (is_readable(glade_filename))
		return;

	// Try ENGINE_GLADE_PATH from the environment
	const char* const env_path = getenv("INGEN_GLADE_PATH");
	if (env_path && is_readable(env_path)) {
		glade_filename = env_path;
		return;
	}

	// Try the default system installed path
	glade_filename = Shared::data_file_path("ingen_gui.glade");
	if (is_readable(glade_filename))
		return;

	error << "[GladeFactory] Unable to find ingen_gui.glade in " << INGEN_DATA_DIR << endl;
	throw std::runtime_error("Unable to find glade file");
}


Glib::RefPtr<Gnome::Glade::Xml>
GladeFactory::new_glade_reference(const string& toplevel_widget)
{
	if (glade_filename.empty())
		find_glade_file();

	try {
		if (toplevel_widget.empty())
			return Gnome::Glade::Xml::create(glade_filename);
		else
			return Gnome::Glade::Xml::create(glade_filename, toplevel_widget.c_str());
	} catch (const Gnome::Glade::XmlError& ex) {
		error << "[GladeFactory] " << ex.what() << endl;
		throw ex;
	}
}


} // namespace GUI
} // namespace Ingen
