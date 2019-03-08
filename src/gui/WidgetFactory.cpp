/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <fstream>
#include <string>

#include "ingen/Log.hpp"
#include "ingen/runtime_paths.hpp"

#include "WidgetFactory.hpp"

namespace ingen {
namespace gui {

Glib::ustring WidgetFactory::ui_filename = "";

inline static bool
is_readable(const std::string& filename)
{
	std::ifstream fs(filename.c_str());
	const bool fail = fs.fail();
	fs.close();
	return !fail;
}

void
WidgetFactory::find_ui_file()
{
	// Try file in bundle (directory where executable resides)
	ui_filename = ingen::bundle_file_path("ingen_gui.ui");
	if (is_readable(ui_filename)) {
		return;
	}

	// Try ENGINE_UI_PATH from the environment
	const char* const env_path = getenv("INGEN_UI_PATH");
	if (env_path && is_readable(env_path)) {
		ui_filename = env_path;
		return;
	}

	// Try the default system installed path
	ui_filename = ingen::data_file_path("ingen_gui.ui");
	if (is_readable(ui_filename)) {
		return;
	}

	throw std::runtime_error(fmt("Unable to find ingen_gui.ui in %1%\n",
	                             INGEN_DATA_DIR));
}

Glib::RefPtr<Gtk::Builder>
WidgetFactory::create(const std::string& toplevel_widget)
{
	if (ui_filename.empty()) {
		find_ui_file();
	}

	if (toplevel_widget.empty()) {
		return Gtk::Builder::create_from_file(ui_filename);
	} else {
		return Gtk::Builder::create_from_file(ui_filename, toplevel_widget.c_str());
	}
}

} // namespace gui
} // namespace ingen
