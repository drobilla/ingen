/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "App.hpp"
#include "LoadPluginWindow.hpp"
#include "Window.hpp"

#include "ingen/Atom.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Interface.hpp"
#include "ingen/URIs.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/GraphModel.hpp"
#include "ingen/client/PluginModel.hpp"
#include "ingen/paths.hpp"
#include "lilv/lilv.h"
#include "raul/Path.hpp"
#include "raul/Symbol.hpp"

#include <gdk/gdkkeysyms-compat.h>
#include <glibmm/listhandle.h>
#include <glibmm/propertyproxy.h>
#include <glibmm/signalproxy.h>
#include <glibmm/ustring.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/combobox.h>
#include <gtkmm/enums.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/treeiter.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treeviewcolumn.h>
#include <sigc++/adaptors/bind.h>
#include <sigc++/functors/mem_fun.h>
#include <sigc++/signal.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

using std::string;

namespace ingen {

using client::ClientStore;
using client::GraphModel;
using client::PluginModel;

namespace gui {

LoadPluginWindow::LoadPluginWindow(BaseObjectType*                   cobject,
                                   const Glib::RefPtr<Gtk::Builder>& xml)
	: Window(cobject)
{
	xml->get_widget("load_plugin_plugins_treeview", _plugins_treeview);
	xml->get_widget("load_plugin_polyphonic_checkbutton", _polyphonic_checkbutton);
	xml->get_widget("load_plugin_name_entry", _name_entry);
	xml->get_widget("load_plugin_add_button", _add_button);
	xml->get_widget("load_plugin_close_button", _close_button);

	xml->get_widget("load_plugin_filter_combo", _filter_combo);
	xml->get_widget("load_plugin_search_entry", _search_entry);

	// Set up the plugins list
	_plugins_liststore = Gtk::ListStore::create(_plugins_columns);
	_plugins_treeview->set_model(_plugins_liststore);
	_plugins_treeview->append_column("_Name", _plugins_columns._col_name);
	_plugins_treeview->append_column("_Type", _plugins_columns._col_type);
	_plugins_treeview->append_column("_Project", _plugins_columns._col_project);
	_plugins_treeview->append_column("_Author", _plugins_columns._col_author);
	_plugins_treeview->append_column("_URI", _plugins_columns._col_uri);

	// This could be nicer.. store the TreeViewColumns locally maybe?
	_plugins_treeview->get_column(0)->set_sort_column(_plugins_columns._col_name);
	_plugins_treeview->get_column(1)->set_sort_column(_plugins_columns._col_type);
	_plugins_treeview->get_column(2)->set_sort_column(_plugins_columns._col_project);
	_plugins_treeview->get_column(2)->set_sort_column(_plugins_columns._col_author);
	_plugins_treeview->get_column(3)->set_sort_column(_plugins_columns._col_uri);
	for (int i = 0; i < 5; ++i) {
		_plugins_treeview->get_column(i)->set_resizable(true);
	}

	// Set up the search criteria combobox
	_criteria_liststore = Gtk::ListStore::create(_criteria_columns);
	_filter_combo->set_model(_criteria_liststore);

	Gtk::TreeModel::iterator iter = _criteria_liststore->append();
	Gtk::TreeModel::Row      row  = *iter;
	row[_criteria_columns._col_label] = "Name contains";
	row[_criteria_columns._col_criteria] = CriteriaColumns::Criteria::NAME;
	_filter_combo->set_active(iter);

	row = *(iter = _criteria_liststore->append());
	row[_criteria_columns._col_label] = "Type contains";
	row[_criteria_columns._col_criteria] = CriteriaColumns::Criteria::TYPE;

	row = *(iter = _criteria_liststore->append());
	row[_criteria_columns._col_label] = "Project contains";
	row[_criteria_columns._col_criteria] = CriteriaColumns::Criteria::PROJECT;

	row = *(iter = _criteria_liststore->append());
	row[_criteria_columns._col_label] = "Author contains";
	row[_criteria_columns._col_criteria] = CriteriaColumns::Criteria::AUTHOR;

	row = *(iter = _criteria_liststore->append());
	row[_criteria_columns._col_label] = "URI contains";
	row[_criteria_columns._col_criteria] = CriteriaColumns::Criteria::URI;
	_filter_combo->pack_start(_criteria_columns._col_label);

	_add_button->signal_clicked().connect(
		sigc::mem_fun(this, &LoadPluginWindow::add_clicked));
	_close_button->signal_clicked().connect(
		sigc::mem_fun(this, &Window::hide));
	_plugins_treeview->signal_row_activated().connect(
		sigc::mem_fun(this, &LoadPluginWindow::plugin_activated));
	_search_entry->signal_activate().connect(
		sigc::mem_fun(this, &LoadPluginWindow::add_clicked));
	_search_entry->signal_changed().connect(
		sigc::mem_fun(this, &LoadPluginWindow::filter_changed));
	_name_entry->signal_changed().connect(
		sigc::mem_fun(this, &LoadPluginWindow::name_changed));

	_search_entry->signal_icon_release().connect(
		sigc::mem_fun(this, &LoadPluginWindow::name_cleared));

	_selection = _plugins_treeview->get_selection();
	_selection->set_mode(Gtk::SELECTION_MULTIPLE);
	_selection->signal_changed().connect(
		sigc::mem_fun(this, &LoadPluginWindow::plugin_selection_changed));

	//m_add_button->grab_default();
}

void
LoadPluginWindow::present(const std::shared_ptr<const GraphModel>& graph,
                          const Properties&                        data)
{
	set_graph(graph);
	_initial_data = data;
	Gtk::Window::present();
}

/** Called every time the user types into the name input box.
 * Used to display warning messages, and enable/disable the OK button.
 */
void
LoadPluginWindow::name_changed()
{
	// Toggle add button sensitivity according name legality
	if (_selection->get_selected_rows().size() == 1) {
		const string sym = _name_entry->get_text();
		if (!raul::Symbol::is_valid(sym)) {
			_add_button->property_sensitive() = false;
		} else if (_app->store()->find(_graph->path().child(raul::Symbol(sym)))
		           != _app->store()->end()) {
			_add_button->property_sensitive() = false;
		} else {
			_add_button->property_sensitive() = true;
		}
	}
}

void
LoadPluginWindow::name_cleared(Gtk::EntryIconPosition pos, const GdkEventButton* event)
{
	_search_entry->set_text("");
}

/** Sets the graph controller for this window and initializes everything.
 *
 * This function MUST be called before using the window in any way!
 */
void
LoadPluginWindow::set_graph(const std::shared_ptr<const GraphModel>& graph)
{
	if (_graph) {
		_graph = graph;
		plugin_selection_changed();
	} else {
		_graph = graph;
	}
}

/** Populates the plugin list on the first show.
 *
 * This is done here instead of construction time as the list population is
 * really expensive and bogs down creation of a graph.  This is especially
 * important when many graph notifications are sent at one time from the
 * engine.
 */
void
LoadPluginWindow::on_show()
{
	if (!_has_shown) {
		_app->store()->signal_new_plugin().connect(
			sigc::mem_fun(this, &LoadPluginWindow::add_plugin));
		_has_shown = true;
	}

	if (_refresh_list) {
		set_plugins(_app->store()->plugins());
		_refresh_list = false;
	}

	Gtk::Window::on_show();
}

void
LoadPluginWindow::set_plugins(
    const std::shared_ptr<const ClientStore::Plugins>& plugins)
{
	_rows.clear();
	_plugins_liststore->clear();

	for (const auto& p : *plugins) {
		add_plugin(p.second);
	}

	_plugins_liststore->set_sort_column(1, Gtk::SORT_ASCENDING);
	_plugins_treeview->columns_autosize();
}

void
LoadPluginWindow::new_plugin(const std::shared_ptr<const PluginModel>& pm)
{
	if (is_visible()) {
		add_plugin(pm);
	} else {
		_refresh_list = true;
	}
}

static std::string
get_project_name(const std::shared_ptr<const PluginModel>& plugin)
{
	std::string name;
	if (plugin->lilv_plugin()) {
		LilvNode* project = lilv_plugin_get_project(plugin->lilv_plugin());
		if (!project) {
			return "";
		}

		LilvNode* doap_name = lilv_new_uri(
			plugin->lilv_world(), "http://usefulinc.com/ns/doap#name");
		LilvNodes* names = lilv_world_find_nodes(
			plugin->lilv_world(), project, doap_name, nullptr);

		if (names) {
			name = lilv_node_as_string(lilv_nodes_get_first(names));
		}

		lilv_nodes_free(names);
		lilv_node_free(doap_name);
		lilv_node_free(project);
	}
	return name;
}

static std::string
get_author_name(const std::shared_ptr<const PluginModel>& plugin)
{
	std::string name;
	if (plugin->lilv_plugin()) {
		LilvNode* author = lilv_plugin_get_author_name(plugin->lilv_plugin());
		if (author) {
			name = lilv_node_as_string(author);
		}
		lilv_node_free(author);
	}
	return name;
}

void
LoadPluginWindow::set_row(Gtk::TreeModel::Row&                      row,
                          const std::shared_ptr<const PluginModel>& plugin)
{
	const URIs&       uris = _app->uris();
	const Atom& name = plugin->get_property(uris.doap_name);
	if (name.is_valid() && name.type() == uris.forge.String) {
		row[_plugins_columns._col_name] = name.ptr<char>();
	}

	if (uris.lv2_Plugin == plugin->type()) {
		row[_plugins_columns._col_type] = lilv_node_as_string(
			lilv_plugin_class_get_label(
				lilv_plugin_get_class(plugin->lilv_plugin())));

		row[_plugins_columns._col_project] = get_project_name(plugin);
		row[_plugins_columns._col_author]  = get_author_name(plugin);
	} else if (uris.ingen_Internal == plugin->type()) {
		row[_plugins_columns._col_type]    = "Internal";
		row[_plugins_columns._col_project] = "Ingen";
		row[_plugins_columns._col_author]  = "David Robillard";
	} else if (uris.ingen_Graph == plugin->type()) {
		row[_plugins_columns._col_type] = "Graph";
	} else {
		row[_plugins_columns._col_type] = "";
	}

	row[_plugins_columns._col_uri]    = plugin->uri().string();
	row[_plugins_columns._col_plugin] = plugin;
}

void
LoadPluginWindow::add_plugin(const std::shared_ptr<const PluginModel>& plugin)
{
	if (plugin->lilv_plugin() && lilv_plugin_is_replaced(plugin->lilv_plugin())) {
		return;
	}

	Gtk::TreeModel::iterator iter = _plugins_liststore->append();
	Gtk::TreeModel::Row      row  = *iter;
	_rows.emplace(plugin->uri(), iter);

	set_row(row, plugin);

	plugin->signal_property().connect(
		sigc::bind<0>(sigc::mem_fun(this, &LoadPluginWindow::plugin_property_changed),
		              plugin->uri()));
}

///// Event Handlers //////

void
LoadPluginWindow::plugin_activated(const Gtk::TreeModel::Path& path,
                                   Gtk::TreeViewColumn*        col)
{
	add_clicked();
}

void
LoadPluginWindow::plugin_selection_changed()
{
	size_t n_selected = _selection->get_selected_rows().size();
	if (n_selected == 0) {
		_name_offset = 0;
		_name_entry->set_text("");
		_name_entry->set_sensitive(false);
	} else if (n_selected == 1) {
		Gtk::TreeModel::iterator iter = _plugins_liststore->get_iter(
			*_selection->get_selected_rows().begin());
		if (iter) {
			Gtk::TreeModel::Row row = *iter;
			auto                p = row.get_value(_plugins_columns._col_plugin);
			_name_offset = _app->store()->child_name_offset(
				_graph->path(), p->default_block_symbol());
			_name_entry->set_text(generate_module_name(p, _name_offset));
			_name_entry->set_sensitive(true);
		} else {
			_name_offset = 0;
			_name_entry->set_text("");
			_name_entry->set_sensitive(false);
		}
	} else {
		_name_entry->set_text("");
		_name_entry->set_sensitive(false);
	}
}

/** Generate an automatic name for this Node.
 *
 * Offset is an offset of the number that will be appended to the plugin's
 * label, needed if the user adds multiple plugins faster than the engine
 * sends the notification back.
 */
string
LoadPluginWindow::generate_module_name(
    const std::shared_ptr<const PluginModel>& plugin,
    int                                       offset)
{
	std::stringstream ss;
	ss << plugin->default_block_symbol();
	if (offset != 0) {
		ss << "_" << offset;
	}
	return ss.str();
}

void
LoadPluginWindow::load_plugin(const Gtk::TreeModel::iterator& iter)
{
	const URIs&         uris       = _app->uris();
	Gtk::TreeModel::Row row        = *iter;
	auto                plugin     = row.get_value(_plugins_columns._col_plugin);
	bool                polyphonic = _polyphonic_checkbutton->get_active();
	string              name       = _name_entry->get_text();

	if (name.empty()) {
		name = generate_module_name(plugin, _name_offset);
	}

	if (name.empty() || !raul::Symbol::is_valid(name)) {
		Gtk::MessageDialog dialog(
			*this,
			"Unable to choose a default name, please provide one",
			false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);

		dialog.run();
	} else {
		raul::Path path  = _graph->path().child(raul::Symbol::symbolify(name));
		Properties props = _initial_data;
		props.emplace(uris.rdf_type, Property(uris.ingen_Block));
		props.emplace(uris.lv2_prototype, _app->forge().make_urid(plugin->uri()));
		props.emplace(uris.ingen_polyphonic, _app->forge().make(polyphonic));
		_app->interface()->put(path_to_uri(path), props);

		if (_selection->get_selected_rows().size() == 1) {
			_name_offset = (_name_offset == 0) ? 2 : _name_offset + 1;
			_name_entry->set_text(generate_module_name(plugin, _name_offset));
		}

		// Cascade next block
		Atom& x = _initial_data.find(uris.ingen_canvasX)->second;
		x = _app->forge().make(x.get<float>() + 20.0f);
		Atom& y = _initial_data.find(uris.ingen_canvasY)->second;
		y = _app->forge().make(y.get<float>() + 20.0f);
	}
}

void
LoadPluginWindow::add_clicked()
{
	_selection->selected_foreach_iter(
		sigc::mem_fun(*this, &LoadPluginWindow::load_plugin));
}

void
LoadPluginWindow::filter_changed()
{
	_rows.clear();
	_plugins_liststore->clear();
	string search = _search_entry->get_text();
	transform(search.begin(), search.end(), search.begin(), ::toupper);

	// Get selected criteria
	const Gtk::TreeModel::Row row = *(_filter_combo->get_active());
	CriteriaColumns::Criteria criteria = row[_criteria_columns._col_criteria];

	string field;

	Gtk::TreeModel::Row      model_row;
	Gtk::TreeModel::iterator model_iter;
	size_t                   num_visible = 0;
	const URIs&              uris        = _app->uris();

	for (const auto& p : *_app->store()->plugins()) {
		const auto  plugin = p.second;
		const Atom& name   = plugin->get_property(uris.doap_name);

		switch (criteria) {
		case CriteriaColumns::Criteria::NAME:
			if (name.is_valid() && name.type() == uris.forge.String) {
				field = name.ptr<char>();
			}
			break;
		case CriteriaColumns::Criteria::TYPE:
			if (plugin->lilv_plugin()) {
				field = lilv_node_as_string(
					lilv_plugin_class_get_label(
						lilv_plugin_get_class(plugin->lilv_plugin())));
			}
			break;
		case CriteriaColumns::Criteria::PROJECT:
			field = get_project_name(plugin);
			break;
		case CriteriaColumns::Criteria::AUTHOR:
			field = get_author_name(plugin);
			break;
		case CriteriaColumns::Criteria::URI:
			field = plugin->uri();
			break;
		}

		transform(field.begin(), field.end(), field.begin(), ::toupper);

		if (field.find(search) != string::npos) {
			model_iter = _plugins_liststore->append();
			model_row = *model_iter;
			set_row(model_row, plugin);
			++num_visible;
		}
	}

	if (num_visible == 1) {
		_selection->unselect_all();
		_selection->select(model_iter);
	}
}

bool
LoadPluginWindow::on_key_press_event(GdkEventKey* event)
{
	if (event->keyval == GDK_w && event->state & GDK_CONTROL_MASK) {
		hide();
		return true;
	} else {
		return Gtk::Window::on_key_press_event(event);
	}
}

void
LoadPluginWindow::plugin_property_changed(const URI&  plugin,
                                          const URI&  predicate,
                                          const Atom& value)
{
	const URIs& uris = _app->uris();
	if (predicate == uris.doap_name) {
		Rows::const_iterator i = _rows.find(plugin);
		if (i != _rows.end() && value.type() == uris.forge.String) {
			(*i->second)[_plugins_columns._col_name] = value.ptr<char>();
		}
	}
}

} // namespace gui
} // namespace ingen
