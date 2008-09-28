/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef LOADPLUGINWINDOW_H
#define LOADPLUGINWINDOW_H

#include <libglademm/xml.h>
#include <libglademm.h>
#include <gtkmm.h>
#include <raul/SharedPtr.hpp>
#include <raul/Table.hpp>
#include "interface/GraphObject.hpp"
#include "client/PatchModel.hpp"
#include "client/PluginModel.hpp"
#include "client/ClientStore.hpp"
using Ingen::Client::PluginModel;
using Ingen::Client::PatchModel;
using namespace Ingen::Shared;

namespace Ingen {
namespace GUI {
	

// Gtkmm _really_ needs to add some helper to abstract away this stupid nonsense

/** Columns for the plugin list in the load plugin window.
 *
 * \ingroup GUI
 */
class ModelColumns : public Gtk::TreeModel::ColumnRecord
{
public:
  ModelColumns() {
	  add(_col_icon);
	  add(_col_name);
	  add(_col_type);
	  add(_col_uri);
	  add(_col_label);
	  //add(_col_library);
	  //add(_col_label);
	  add(_col_plugin_model);
  }

  Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > _col_icon;
  Gtk::TreeModelColumn<Glib::ustring> _col_name;
  Gtk::TreeModelColumn<Glib::ustring> _col_type;
  Gtk::TreeModelColumn<Glib::ustring> _col_uri;

  // Not displayed:
  Gtk::TreeModelColumn<Glib::ustring>            _col_label;
  Gtk::TreeModelColumn<SharedPtr<PluginModel> > _col_plugin_model;
};


/** Column for the criteria combo box in the load plugin window.
 *
 * \ingroup GUI
 */
class CriteriaColumns : public Gtk::TreeModel::ColumnRecord
{
public:
	enum Criteria { NAME, TYPE, URI, };
	
	CriteriaColumns() { add(_col_label); add(_col_criteria); }
	
	Gtk::TreeModelColumn<Glib::ustring> _col_label;
	Gtk::TreeModelColumn<Criteria>      _col_criteria;
};


/** 'Load Plugin' window.
 *
 * Loaded by glade as a derived object.
 *
 * \ingroup GUI
 */
class LoadPluginWindow : public Gtk::Window
{
public:
	LoadPluginWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);

	void set_patch(SharedPtr<PatchModel> patch);
	void set_plugins(SharedPtr<const Client::ClientStore::Plugins> plugins);

	void add_plugin(SharedPtr<PluginModel> plugin);

	void present(SharedPtr<PatchModel> patch, GraphObject::Variables data);

protected:
	void on_show();
	bool on_key_press_event(GdkEventKey* event);
	
private:
	void add_clicked();
	//void close_clicked();
	//void ok_clicked();
	void filter_changed();
	void clear_clicked();
	void name_changed();
	
	void new_plugin(SharedPtr<PluginModel> plugin);

	int plugin_compare(const Gtk::TreeModel::iterator& a,
	                   const Gtk::TreeModel::iterator& b);

	void plugin_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* col);
	void plugin_selection_changed();
	string generate_module_name(int offset = 0);

	GraphObject::Variables _initial_data;

	SharedPtr<PatchModel> _patch;

	Glib::RefPtr<Gtk::ListStore> _plugins_liststore;
	ModelColumns                 _plugins_columns;

	Glib::RefPtr<Gtk::ListStore> _criteria_liststore;
	CriteriaColumns              _criteria_columns;
	
	Glib::RefPtr<Gtk::TreeSelection> _selection;
	
	int _plugin_name_offset; // see comments for generate_plugin_name
	
	bool              _has_shown;
	bool              _refresh_list;
	Gtk::TreeView*    _plugins_treeview;
	Gtk::CheckButton* _polyphonic_checkbutton;
	Gtk::Entry*       _node_name_entry;
	Gtk::Button*      _clear_button;
	Gtk::Button*      _add_button;
	//Gtk::Button*      _close_button;
	//Gtk::Button*      _ok_button;
	Gtk::ComboBox*    _filter_combo;
	Gtk::Entry*       _search_entry;
};


} // namespace GUI
} // namespace Ingen

#endif // LOADPLUGINWINDOW_H
