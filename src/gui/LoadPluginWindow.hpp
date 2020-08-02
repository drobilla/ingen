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

#ifndef INGEN_GUI_LOADPLUGINWINDOW_HPP
#define INGEN_GUI_LOADPLUGINWINDOW_HPP

#include "Window.hpp"

#include "ingen/Node.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/memory.hpp"
#include "ingen_config.h"

#include <gtkmm/builder.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treeview.h>

#include <map>
#include <string>

namespace ingen {

namespace client {
class GraphModel;
class PluginModel;
} // namespace client

namespace gui {

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

	void set_graph(const SPtr<const client::GraphModel>& graph);

	void set_plugins(const SPtr<const client::ClientStore::Plugins>& plugins);

	void add_plugin(const SPtr<const client::PluginModel>& plugin);

	void present(const SPtr<const client::GraphModel>& graph,
	             const Properties&                     data);

protected:
	void on_show() override;
	bool on_key_press_event(GdkEventKey* event) override;

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
		Gtk::TreeModelColumn< SPtr<const client::PluginModel> > _col_plugin;
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
	void name_cleared(Gtk::EntryIconPosition pos, const GdkEventButton* event);

	void set_row(Gtk::TreeModel::Row&                   row,
	             const SPtr<const client::PluginModel>& plugin);

	void new_plugin(const SPtr<const client::PluginModel>& pm);

	void plugin_property_changed(const URI&  plugin,
	                             const URI&  predicate,
	                             const Atom& value);

	void plugin_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* col);
	void plugin_selection_changed();

	static std::string
	generate_module_name(const SPtr<const client::PluginModel>& plugin,
	                     int                                    offset = 0);

	void load_plugin(const Gtk::TreeModel::iterator& iter);

	Properties _initial_data;

	SPtr<const client::GraphModel> _graph;

	using Rows = std::map<URI, Gtk::TreeModel::iterator>;
	Rows _rows;

	Glib::RefPtr<Gtk::ListStore> _plugins_liststore;
	ModelColumns                 _plugins_columns;

	Glib::RefPtr<Gtk::ListStore> _criteria_liststore;
	CriteriaColumns              _criteria_columns;

	Glib::RefPtr<Gtk::TreeSelection> _selection;

	int _name_offset = 0; // see comments for generate_plugin_name

	bool              _has_shown              = false;
	bool              _refresh_list           = true;
	Gtk::TreeView*    _plugins_treeview       = nullptr;
	Gtk::CheckButton* _polyphonic_checkbutton = nullptr;
	Gtk::Entry*       _name_entry             = nullptr;
	Gtk::Button*      _close_button           = nullptr;
	Gtk::Button*      _add_button             = nullptr;
	Gtk::ComboBox*    _filter_combo           = nullptr;
	Gtk::Entry*       _search_entry           = nullptr;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_LOADPLUGINWINDOW_HPP
