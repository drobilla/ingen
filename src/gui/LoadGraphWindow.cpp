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

#include "LoadGraphWindow.hpp"

#include "App.hpp"
#include "ThreadedLoader.hpp"

#include "ingen/Atom.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/FilePath.hpp"
#include "ingen/Forge.hpp"
#include "ingen/URI.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/GraphModel.hpp"
#include "ingen/runtime_paths.hpp"
#include "raul/Path.hpp"

#include <boost/optional/optional.hpp>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <glibmm/propertyproxy.h>
#include <glibmm/refptr.h>
#include <glibmm/signalproxy.h>
#include <glibmm/slisthandle.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/filefilter.h>
#include <gtkmm/label.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/window.h>
#include <sigc++/adaptors/bind.h>
#include <sigc++/functors/mem_fun.h>

#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

namespace ingen {

using client::GraphModel;

namespace gui {

LoadGraphWindow::LoadGraphWindow(BaseObjectType*                   cobject,
                                 const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::FileChooserDialog(cobject)
{
	xml->get_widget("load_graph_symbol_label", _symbol_label);
	xml->get_widget("load_graph_symbol_entry", _symbol_entry);
	xml->get_widget("load_graph_ports_label", _ports_label);
	xml->get_widget("load_graph_merge_ports_radio", _merge_ports_radio);
	xml->get_widget("load_graph_insert_ports_radio", _insert_ports_radio);
	xml->get_widget("load_graph_poly_voices_radio", _poly_voices_radio);
	xml->get_widget("load_graph_poly_from_file_radio", _poly_from_file_radio);
	xml->get_widget("load_graph_poly_spinbutton", _poly_spinbutton);
	xml->get_widget("load_graph_ok_button", _ok_button);
	xml->get_widget("load_graph_cancel_button", _cancel_button);

	_cancel_button->signal_clicked().connect(
		sigc::mem_fun(this, &LoadGraphWindow::cancel_clicked));
	_ok_button->signal_clicked().connect(
		sigc::mem_fun(this, &LoadGraphWindow::ok_clicked));
	_merge_ports_radio->signal_toggled().connect(
		sigc::mem_fun(this, &LoadGraphWindow::merge_ports_selected));
	_insert_ports_radio->signal_toggled().connect(
		sigc::mem_fun(this, &LoadGraphWindow::insert_ports_selected));
	_poly_from_file_radio->signal_toggled().connect(
		sigc::bind(sigc::mem_fun(_poly_spinbutton, &Gtk::SpinButton::set_sensitive),
		           false));
	_poly_voices_radio->signal_toggled().connect(
		sigc::bind(sigc::mem_fun(_poly_spinbutton, &Gtk::SpinButton::set_sensitive),
		           true));

	signal_selection_changed().connect(
		sigc::mem_fun(this, &LoadGraphWindow::selection_changed));

	Gtk::FileFilter file_filter;
	file_filter.add_pattern("*.ttl");
	file_filter.set_name("Ingen graph files (*.ttl)");
	add_filter(file_filter);

	Gtk::FileFilter bundle_filter;
	bundle_filter.add_pattern("*.ingen");
	bundle_filter.set_name("Ingen bundles (*.ingen)");
	add_filter(bundle_filter);

	property_select_multiple() = true;

	// Add global examples directory to "shortcut folders" (bookmarks)
	const FilePath examples_dir = ingen::data_file_path("graphs");
	if (Glib::file_test(examples_dir, Glib::FILE_TEST_IS_DIR)) {
		add_shortcut_folder(examples_dir.string());
	}
}

void
LoadGraphWindow::present(const std::shared_ptr<const GraphModel>& graph,
                         bool                                     import,
                         const Properties&                        data)
{
	_import = import;
	set_graph(graph);
	_symbol_label->property_visible() = !import;
	_symbol_entry->property_visible() = !import;
	_ports_label->property_visible() = _import;
	_merge_ports_radio->property_visible() = _import;
	_insert_ports_radio->property_visible() = _import;
	_initial_data = data;
	Gtk::Window::present();
}

/** Sets the graph model for this window and initializes everything.
 *
 * This function MUST be called before using the window in any way!
 */
void
LoadGraphWindow::set_graph(const std::shared_ptr<const GraphModel>& graph)
{
	_graph = graph;
	_symbol_entry->set_text("");
	_symbol_entry->set_sensitive(!_import);
	_poly_spinbutton->set_value(graph->internal_poly());
}

void
LoadGraphWindow::on_show()
{
	const Atom& dir = _app->world().conf().option("graph-directory");
	if (dir.is_valid()) {
		set_current_folder(dir.ptr<char>());
	}
	Gtk::FileChooserDialog::on_show();
}

void
LoadGraphWindow::merge_ports_selected()
{
	_merge_ports = true;
}

void
LoadGraphWindow::insert_ports_selected()
{
	_merge_ports = false;
}

void
LoadGraphWindow::ok_clicked()
{
	if (!_graph) {
		hide();
		return;
	}

	const URIs& uris = _app->uris();

	if (_poly_voices_radio->get_active()) {
		_initial_data.emplace(
			uris.ingen_polyphony,
			_app->forge().make(_poly_spinbutton->get_value_as_int()));
	}

	if (get_uri().empty()) {
		return;
	}

	if (_import) {
		// If unset load_graph will load value
		boost::optional<raul::Path>   parent;
		boost::optional<raul::Symbol> symbol;
		if (!_graph->path().is_root()) {
			parent = _graph->path().parent();
			symbol = _graph->symbol();
		}

		_app->loader()->load_graph(
			true, FilePath(get_filename()), parent, symbol, _initial_data);

	} else {
		std::list<Glib::ustring> uri_list = get_filenames();
		for (const auto& u : uri_list) {
			// Cascade
			Atom& x = _initial_data.find(uris.ingen_canvasX)->second;
			x = _app->forge().make(x.get<float>() + 20.0f);
			Atom& y = _initial_data.find(uris.ingen_canvasY)->second;
			y = _app->forge().make(y.get<float>() + 20.0f);

			raul::Symbol symbol(symbol_from_filename(u));
			if (uri_list.size() == 1 && !_symbol_entry->get_text().empty()) {
				symbol = raul::Symbol::symbolify(_symbol_entry->get_text());
			}

			symbol = avoid_symbol_clash(symbol);

			_app->loader()->load_graph(
				false, FilePath(URI(u).path()), _graph->path(), symbol, _initial_data);
		}
	}

	_graph.reset();
	hide();

	_app->world().conf().set(
		"graph-directory",
		_app->world().forge().alloc(get_current_folder()));
}

void
LoadGraphWindow::cancel_clicked()
{
	_graph.reset();
	hide();
}

raul::Symbol
LoadGraphWindow::symbol_from_filename(const Glib::ustring& filename)
{
	std::string symbol_str = Glib::path_get_basename(get_filename());
	symbol_str = symbol_str.substr(0, symbol_str.find('.'));
	return raul::Symbol::symbolify(symbol_str);
}

raul::Symbol
LoadGraphWindow::avoid_symbol_clash(const raul::Symbol& symbol)
{
	unsigned offset = _app->store()->child_name_offset(
		_graph->path(), symbol);

	if (offset != 0) {
		std::stringstream ss;
		ss << symbol << "_" << offset;
		return raul::Symbol(ss.str());
	}

	return symbol;
}

void
LoadGraphWindow::selection_changed()
{
	if (_import) {
		return;
	}

	if (get_filenames().size() != 1) {
		_symbol_entry->set_text("");
		_symbol_entry->set_sensitive(false);
	} else {
		_symbol_entry->set_text(
			avoid_symbol_clash(symbol_from_filename(get_filename())).c_str());
		_symbol_entry->set_sensitive(true);
	}
}

} // namespace gui
} // namespace ingen
