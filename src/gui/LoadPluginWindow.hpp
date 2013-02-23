/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_LOADPLUGINWINDOW_HPP
#define INGEN_GUI_LOADPLUGINWINDOW_HPP

#include <map>
#include <string>

#include <gtkmm/builder.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treeview.h>

#include "ingen/Node.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/types.hpp"
#include "ingen_config.h"

#include "Window.hpp"

namespace Ingen {

namespace Client {
class GraphModel;
class PluginModel;
}

namespace GUI {

/** 'Load Plugin' window.
 *
 * Loaded from XML as a derived object.
 *
 * \ingroup GUI
 */
class LoadPluginWindow : public Window
{
public:
	LoadPluginWindow(BaseObjectType*                   cobject,
	                 const Glib::RefPtr<Gtk::Builder>& xml);

	void set_graph(SPtr<const Client::GraphModel> graph);
	void set_plugins(SPtr<const Client::ClientStore::Plugins> plugins);

	void add_plugin(SPtr<const Client::PluginModel> plugin);

	void present(SPtr<const Client::GraphModel> graph,
	             Node::Properties               data);

protected:
	void on_show();
	bool on_key_press_event(GdkEventKey* event);

private:
	/** Columns for the plugin list */
	class ModelColumns : public Gtk::TreeModel::ColumnRecord {
	public:
		ModelColumns() {
			add(_col_name);
			add(_col_type);
			add(_col_project);
			add(_col_author);
			add(_col_uri);
			add(_col_plugin);
		}

		Gtk::TreeModelColumn<Glib::ustring> _col_name;
		Gtk::TreeModelColumn<Glib::ustring> _col_type;
		Gtk::TreeModelColumn<Glib::ustring> _col_project;
		Gtk::TreeModelColumn<Glib::ustring> _col_author;
		Gtk::TreeModelColumn<Glib::ustring> _col_uri;

		// Not displayed:
		Gtk::TreeModelColumn< SPtr<const Client::PluginModel> > _col_plugin;
	};

	/** Column for the filter criteria combo box. */
	class CriteriaColumns : public Gtk::TreeModel::ColumnRecord {
	public:
		enum class Criteria { NAME, TYPE, PROJECT, AUTHOR, URI, };

		CriteriaColumns() {
			add(_col_label);
			add(_col_criteria);
		}

		Gtk::TreeModelColumn<Glib::ustring> _col_label;
		Gtk::TreeModelColumn<Criteria>      _col_criteria;
	};

	void add_clicked();
	void filter_changed();
	void clear_clicked();
	void name_changed();
#ifdef HAVE_NEW_GTKMM
	void name_cleared(Gtk::EntryIconPosition pos, const GdkEventButton* event);
#endif

	void set_row(Gtk::TreeModel::Row&            row,
	             SPtr<const Client::PluginModel> plugin);

	void new_plugin(SPtr<const Client::PluginModel> plugin);

	void plugin_property_changed(const Raul::URI& plugin,
	                             const Raul::URI& predicate,
	                             const Atom&      value);

	void plugin_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* col);
	void plugin_selection_changed();

	std::string generate_module_name(SPtr<const Client::PluginModel> plugin,
	                                 int                             offset=0);

	void load_plugin(const Gtk::TreeModel::iterator& iter);

	Node::Properties _initial_data;

	SPtr<const Client::GraphModel> _graph;

	typedef std::map<Raul::URI, Gtk::TreeModel::iterator> Rows;
	Rows _rows;

	Glib::RefPtr<Gtk::ListStore> _plugins_liststore;
	ModelColumns                 _plugins_columns;

	Glib::RefPtr<Gtk::ListStore> _criteria_liststore;
	CriteriaColumns              _criteria_columns;

	Glib::RefPtr<Gtk::TreeSelection> _selection;

	int _name_offset; // see comments for generate_plugin_name

	bool              _has_shown;
	bool              _refresh_list;
	Gtk::TreeView*    _plugins_treeview;
	Gtk::CheckButton* _polyphonic_checkbutton;
	Gtk::Entry*       _name_entry;
	Gtk::Button*      _close_button;
	Gtk::Button*      _add_button;
	Gtk::ComboBox*    _filter_combo;
	Gtk::Entry*       _search_entry;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_LOADPLUGINWINDOW_HPP
