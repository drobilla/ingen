/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#include <sys/types.h>
#include <dirent.h>
#include <cassert>
#include <boost/optional.hpp>
#include <glibmm/miscutils.h>
#include "interface/EngineInterface.hpp"
#include "shared/LV2URIMap.hpp"
#include "client/NodeModel.hpp"
#include "client/PatchModel.hpp"
#include "client/ClientStore.hpp"
#include "shared/runtime_paths.hpp"
#include "App.hpp"
#include "LoadSubpatchWindow.hpp"
#include "PatchView.hpp"
#include "Configuration.hpp"
#include "ThreadedLoader.hpp"

using boost::optional;
using namespace std;
using namespace Raul;

namespace Ingen {
namespace GUI {


LoadSubpatchWindow::LoadSubpatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::FileChooserDialog(cobject)
{
	xml->get_widget("load_subpatch_symbol_entry", _symbol_entry);
	xml->get_widget("load_subpatch_poly_voices_radio", _poly_voices_radio);
	xml->get_widget("load_subpatch_poly_from_file_radio", _poly_from_file_radio);
	xml->get_widget("load_subpatch_poly_spinbutton", _poly_spinbutton);
	xml->get_widget("load_subpatch_ok_button", _ok_button);
	xml->get_widget("load_subpatch_cancel_button", _cancel_button);

	_poly_voices_radio->signal_toggled().connect(sigc::mem_fun(this, &LoadSubpatchWindow::enable_poly_spinner));
	_ok_button->signal_clicked().connect(sigc::mem_fun(this, &LoadSubpatchWindow::ok_clicked));
	_cancel_button->signal_clicked().connect(sigc::mem_fun(this, &LoadSubpatchWindow::cancel_clicked));

	Gtk::FileFilter filt;
	filt.add_pattern("*.om");
	filt.set_name("Om patch files (XML, DEPRECATED) (*.om)");
	filt.add_pattern("*.ingen.ttl");
	filt.set_name("Ingen patch files (RDF, *.ingen.ttl)");
	set_filter(filt);

	property_select_multiple() = true;

	// Add global examples directory to "shortcut folders" (bookmarks)
	const string examples_dir = Shared::data_file_path("patches");
	DIR* d = opendir(examples_dir.c_str());
	if (d != NULL) {
		add_shortcut_folder(examples_dir);
		closedir(d);
	}

	signal_selection_changed().connect(
			sigc::mem_fun(this, &LoadSubpatchWindow::selection_changed));
}


void
LoadSubpatchWindow::present(SharedPtr<PatchModel> patch, GraphObject::Properties data)
{
	set_patch(patch);
	_initial_data = data;
	Gtk::Window::present();
}


/** Sets the patch model for this window and initializes everything.
 *
 * This function MUST be called before using the window in any way!
 */
void
LoadSubpatchWindow::set_patch(SharedPtr<PatchModel> patch)
{
	_patch = patch;
	_symbol_entry->set_text(patch->path().symbol());
	_poly_spinbutton->set_value(patch->poly());
}


void
LoadSubpatchWindow::on_show()
{
	if (App::instance().configuration()->patch_folder().length() > 0)
		set_current_folder(App::instance().configuration()->patch_folder());
	Gtk::FileChooserDialog::on_show();
}


///// Event Handlers //////


void
LoadSubpatchWindow::disable_poly_spinner()
{
	_poly_spinbutton->property_sensitive() = false;
}


void
LoadSubpatchWindow::enable_poly_spinner()
{
	_poly_spinbutton->property_sensitive() = true;
}


void
LoadSubpatchWindow::ok_clicked()
{
	assert(_patch);

	const LV2URIMap& uris = App::instance().uris();

	if (_poly_voices_radio->get_active()) {
		_initial_data.insert(make_pair(uris.ingen_polyphony, (int)_poly_spinbutton->get_value_as_int()));
	}

	std::list<Glib::ustring> uri_list = get_uris();
	for (std::list<Glib::ustring>::iterator i = uri_list.begin(); i != uri_list.end(); ++i) {
		// Cascade
		Atom& x = _initial_data.find(uris.ingenui_canvas_x)->second;
		x = Atom(x.get_float() + 20.0f);
		Atom& y = _initial_data.find(uris.ingenui_canvas_y)->second;
		y = Atom(y.get_float() + 20.0f);

		Raul::Symbol symbol(symbol_from_filename(*i));
		if (uri_list.size() == 1 && _symbol_entry->get_text() != "")
			symbol = Symbol::symbolify(_symbol_entry->get_text());

		symbol = avoid_symbol_clash(symbol);

		App::instance().loader()->load_patch(false, *i, Path("/"),
				_patch->path(), symbol, _initial_data);
	}

	hide();
}


void
LoadSubpatchWindow::cancel_clicked()
{
	hide();
}


Raul::Symbol
LoadSubpatchWindow::symbol_from_filename(const Glib::ustring& filename)
{
	std::string symbol_str = Glib::path_get_basename(get_filename());
	symbol_str = symbol_str.substr(0, symbol_str.find('.'));
	return Raul::Symbol::symbolify(symbol_str);
}


Raul::Symbol
LoadSubpatchWindow::avoid_symbol_clash(const Raul::Symbol& symbol)
{
	unsigned offset = App::instance().store()->child_name_offset(
			_patch->path(), symbol);

	if (offset != 0) {
		std::stringstream ss;
		ss << symbol << "_" << offset + 1;
		return ss.str();
	} else {
		return symbol;
	}
}


void
LoadSubpatchWindow::selection_changed()
{
	if (get_filenames().size() != 1) {
		_symbol_entry->set_text("");
		_symbol_entry->set_sensitive(false);
	} else {
		_symbol_entry->set_text(avoid_symbol_clash(
				symbol_from_filename(get_filename()).c_str()).c_str());
		_symbol_entry->set_sensitive(true);
	}
}


} // namespace GUI
} // namespace Ingen
