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

#include <cassert>
#include <algorithm>
#include <ctype.h>
#include "interface/EngineInterface.hpp"
#include "shared/LV2URIMap.hpp"
#include "client/PatchModel.hpp"
#include "client/ClientStore.hpp"
#include "App.hpp"
#include "LoadPluginWindow.hpp"
#include "PatchWindow.hpp"
#include "PatchView.hpp"
#include "PatchCanvas.hpp"

#define NAME_ENTRY_MULTI_STRING "(multiple values)"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace GUI {

LoadPluginWindow::LoadPluginWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Window(cobject)
	, _plugin_name_offset(0)
	, _has_shown(false)
	, _refresh_list(true)
{
	xml->get_widget("load_plugin_plugins_treeview", _plugins_treeview);
	xml->get_widget("load_plugin_polyphonic_checkbutton", _polyphonic_checkbutton);
	xml->get_widget("load_plugin_name_entry", _node_name_entry);
	xml->get_widget("load_plugin_add_button", _add_button);
	xml->get_widget("load_plugin_close_button", _close_button);

	xml->get_widget("load_plugin_filter_combo", _filter_combo);
	xml->get_widget("load_plugin_search_entry", _search_entry);

	// Set up the plugins list
	_plugins_liststore = Gtk::ListStore::create(_plugins_columns);
	_plugins_treeview->set_model(_plugins_liststore);
	_plugins_treeview->append_column("", _plugins_columns._col_icon);
	_plugins_treeview->append_column("Name", _plugins_columns._col_name);
	_plugins_treeview->append_column("Type", _plugins_columns._col_type);
	_plugins_treeview->append_column("URI", _plugins_columns._col_uri);

	// This could be nicer.. store the TreeViewColumns locally maybe?
	_plugins_treeview->get_column(1)->set_sort_column(_plugins_columns._col_name);
	_plugins_treeview->get_column(2)->set_sort_column(_plugins_columns._col_type);
	_plugins_treeview->get_column(3)->set_sort_column(_plugins_columns._col_uri);
	for (int i=0; i < 3; ++i)
		_plugins_treeview->get_column(i)->set_resizable(true);

	// Set up the search criteria combobox
	_criteria_liststore = Gtk::ListStore::create(_criteria_columns);
	_filter_combo->set_model(_criteria_liststore);
	Gtk::TreeModel::iterator iter = _criteria_liststore->append();
	Gtk::TreeModel::Row row = *iter;
	row[_criteria_columns._col_label] = "Name contains";
	row[_criteria_columns._col_criteria] = CriteriaColumns::NAME;
	_filter_combo->set_active(iter);
	iter = _criteria_liststore->append(); row = *iter;
	row[_criteria_columns._col_label] = "Type contains";
	row[_criteria_columns._col_criteria] = CriteriaColumns::TYPE;
	iter = _criteria_liststore->append(); row = *iter;
	row[_criteria_columns._col_label] = "URI contains";
	row[_criteria_columns._col_criteria] = CriteriaColumns::URI;

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
	_node_name_entry->signal_changed().connect(
			sigc::mem_fun(this, &LoadPluginWindow::name_changed));

#ifdef HAVE_NEW_GTKMM
	_search_entry->signal_icon_release().connect(
			sigc::mem_fun(this, &LoadPluginWindow::name_cleared));
#endif

	_selection = _plugins_treeview->get_selection();
	_selection->set_mode(Gtk::SELECTION_MULTIPLE);
	_selection->signal_changed().connect(
			sigc::mem_fun(this, &LoadPluginWindow::plugin_selection_changed));

	//m_add_button->grab_default();
}


void
LoadPluginWindow::present(SharedPtr<PatchModel> patch, GraphObject::Properties data)
{
	set_patch(patch);
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
		string name = _node_name_entry->get_text();
		if (!Path::is_valid_name(name)) {
			//m_message_label->set_text("Name contains invalid characters.");
			_add_button->property_sensitive() = false;
		} else if (App::instance().store()->find_child(_patch, name)) {
			//m_message_label->set_text("An object already exists with that name.");
			_add_button->property_sensitive() = false;
		} else if (name.length() == 0) {
			//m_message_label->set_text("");
			_add_button->property_sensitive() = false;
		} else {
			//m_message_label->set_text("");
			_add_button->property_sensitive() = true;
		}
	}
}


#ifdef HAVE_NEW_GTKMM
void
LoadPluginWindow::name_cleared(Gtk::EntryIconPosition pos, const GdkEventButton* event)
{
	_search_entry->set_text("");
}
#endif // HAVE_NEW_GTKMM


/** Sets the patch controller for this window and initializes everything.
 *
 * This function MUST be called before using the window in any way!
 */
void
LoadPluginWindow::set_patch(SharedPtr<PatchModel> patch)
{
	if (_patch) {
		_patch = patch;
		plugin_selection_changed();
	} else {
		_patch = patch;
	}

	/*if (patch->poly() <= 1)
		_polyphonic_checkbutton->property_sensitive() = false;
	else
		_polyphonic_checkbutton->property_sensitive() = true;*/
}


/** Populates the plugin list on the first show.
 *
 * This is done here instead of construction time as the list population is
 * really expensive and bogs down creation of a patch.  This is especially
 * important when many patch notifications are sent at one time from the
 * engine.
 */
void
LoadPluginWindow::on_show()
{
	if (!_has_shown) {
		App::instance().store()->signal_new_plugin.connect(
				sigc::mem_fun(this, &LoadPluginWindow::add_plugin));
		_has_shown = true;
	}

	if (_refresh_list) {
		set_plugins(App::instance().store()->plugins());
		_refresh_list = false;
	}

	Gtk::Window::on_show();
}


void
LoadPluginWindow::set_plugins(SharedPtr<const ClientStore::Plugins> m)
{
	_rows.clear();
	_plugins_liststore->clear();

	for (ClientStore::Plugins::const_iterator i = m->begin(); i != m->end(); ++i) {
		add_plugin(i->second);
	}

	_plugins_liststore->set_sort_column(1,
			//Gtk::TreeSortable::DEFAULT_SORT_COLUMN_ID,
			Gtk::SORT_ASCENDING);

	_plugins_treeview->columns_autosize();
}


void
LoadPluginWindow::new_plugin(SharedPtr<PluginModel> pm)
{
	if (is_visible())
		add_plugin(pm);
	else
		_refresh_list = true;
}


void
LoadPluginWindow::add_plugin(SharedPtr<PluginModel> plugin)
{
	const LV2URIMap& uris = App::instance().uris();
	Gtk::TreeModel::iterator iter = _plugins_liststore->append();
	Gtk::TreeModel::Row row = *iter;
	_rows.insert(make_pair(plugin->uri(), iter));

	const Atom& name = plugin->get_property(uris.doap_name);
	if (name.is_valid()) {
		if (name.type() == Atom::STRING)
			row[_plugins_columns._col_name] = name.get_string();
	} else if (plugin->type() == Plugin::LADSPA) {
		App::instance().engine()->request_property(plugin->uri(), uris.doap_name);
	}

	switch (plugin->type()) {
	case Plugin::LV2:
		row[_plugins_columns._col_type] = "LV2";
		break;
	case Plugin::LADSPA:
		row[_plugins_columns._col_type] = "LADSPA";
		break;
	case Plugin::Internal:
		row[_plugins_columns._col_type] = "Internal";
		break;
	case Plugin::Patch:
		row[_plugins_columns._col_type] = "Patch";
		break;
	case Plugin::NIL:
		row[_plugins_columns._col_type] = "?";
		break;
	}

	row[_plugins_columns._col_uri] = plugin->uri().str();
	row[_plugins_columns._col_plugin] = plugin;

	plugin->signal_property.connect(sigc::bind<0>(
			sigc::mem_fun(this, &LoadPluginWindow::plugin_property_changed),
			plugin->uri()));
}



///// Event Handlers //////


void
LoadPluginWindow::plugin_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* col)
{
	add_clicked();
}


void
LoadPluginWindow::plugin_selection_changed()
{
	size_t n_selected = _selection->get_selected_rows().size();
	if (n_selected == 0) {
		_plugin_name_offset = 0;
		_node_name_entry->set_text("");
	} else if (n_selected == 1) {
		Gtk::TreeModel::iterator iter = _plugins_liststore->get_iter(
				*_selection->get_selected_rows().begin());
		if (iter) {
			Gtk::TreeModel::Row row = *iter;
			boost::shared_ptr<PluginModel> p = row.get_value(_plugins_columns._col_plugin);
			_plugin_name_offset = App::instance().store()->child_name_offset(
					_patch->path(), p->default_node_symbol());
			_node_name_entry->set_text(generate_module_name(p, _plugin_name_offset));
		} else {
			_plugin_name_offset = 0;
			_node_name_entry->set_text("");
		}
	} else {
		_node_name_entry->set_text(NAME_ENTRY_MULTI_STRING);
	}
}


/** Generate an automatic name for this Node.
 *
 * Offset is an offset of the number that will be appended to the plugin's
 * label, needed if the user adds multiple plugins faster than the engine
 * sends the notification back.
 */
string
LoadPluginWindow::generate_module_name(SharedPtr<PluginModel> plugin, int offset)
{
	std::stringstream ss;
	ss << plugin->default_node_symbol();
	if (offset != 0)
		ss << "_" << offset + 1;
	return ss.str();
}


void
LoadPluginWindow::load_plugin(const Gtk::TreeModel::iterator& iter)
{
	const LV2URIMap&       uris       = App::instance().uris();
	Gtk::TreeModel::Row    row        = *iter;
	SharedPtr<PluginModel> plugin     = row.get_value(_plugins_columns._col_plugin);
	bool                   polyphonic = _polyphonic_checkbutton->get_active();
	string                 name       = _node_name_entry->get_text();

	if (name.empty() || name == NAME_ENTRY_MULTI_STRING)
		name = generate_module_name(plugin, _plugin_name_offset);

	if (name.empty() || !Symbol::is_valid(name)) {
		Gtk::MessageDialog dialog(*this,
				"Unable to chose a default name for this node.  Please enter a name.",
				false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);

		dialog.run();
	} else {
		Path path = _patch->path().base() + Path::nameify(name);
		Resource::Properties props = _initial_data;
		props.insert(make_pair(uris.rdf_type,         uris.ingen_Node));
		props.insert(make_pair(uris.rdf_instanceOf,   plugin->uri()));
		props.insert(make_pair(uris.ingen_polyphonic, polyphonic));
		App::instance().engine()->put(path, props);

		if (_selection->get_selected_rows().size() == 1)
			_node_name_entry->set_text(generate_module_name(plugin, _plugin_name_offset + 1));

		// Cascade next node
		Atom& x = _initial_data.find(uris.ingenui_canvas_x)->second;
		x = Atom(x.get_float() + 20.0f);
		Atom& y = _initial_data.find(uris.ingenui_canvas_y)->second;
		y = Atom(y.get_float() + 20.0f);
	}
}


void
LoadPluginWindow::add_clicked()
{
	_selection->selected_foreach_iter(
			sigc::mem_fun(*this, &LoadPluginWindow::load_plugin));

	++_plugin_name_offset;
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
	const LV2URIMap&         uris = App::instance().uris();

	for (ClientStore::Plugins::const_iterator i = App::instance().store()->plugins()->begin();
			i != App::instance().store()->plugins()->end(); ++i) {

		const SharedPtr<PluginModel> plugin = (*i).second;
		const Atom& name = plugin->get_property(uris.doap_name);

		switch (criteria) {
		case CriteriaColumns::NAME:
			if (name.is_valid() && name.type() == Atom::STRING)
				field = name.get_string();
			break;
		case CriteriaColumns::TYPE:
			field = plugin->type_uri().str(); break;
		case CriteriaColumns::URI:
			field = plugin->uri().str(); break;
		default:
			throw;
		}

		transform(field.begin(), field.end(), field.begin(), ::toupper);

		if (field.find(search) != string::npos) {
			model_iter = _plugins_liststore->append();
			model_row = *model_iter;

			model_row[_plugins_columns._col_name]   = name.is_valid() ? name.get_string() : "";
			model_row[_plugins_columns._col_type]   = plugin->type_uri().str();
			model_row[_plugins_columns._col_uri]    = plugin->uri().str();
			model_row[_plugins_columns._col_plugin] = plugin;

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
	const LV2URIMap& uris = App::instance().uris();
	if (predicate == uris.doap_name) {
		Rows::const_iterator i = _rows.find(plugin);
		if (i != _rows.end() && value.type() == Atom::STRING)
			(*i->second)[_plugins_columns._col_name] = value.get_string();
	}
}


} // namespace GUI
} // namespace Ingen

