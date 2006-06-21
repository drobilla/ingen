/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */


#ifndef LOADPLUGINWINDOW_H
#define LOADPLUGINWINDOW_H

#include "PluginModel.h"
#include <map>
#include <libglademm/xml.h>
#include <libglademm.h>
#include <gtkmm.h>
#include "util/CountedPtr.h"

using LibOmClient::PluginModel;

namespace OmGtk {
	
class PatchController;

// Gtkmm _really_ needs to add some helper to abstract away this stupid nonsense

/** Columns for the plugin list in the load plugin window.
 *
 * \ingroup OmGtk
 */
class ModelColumns : public Gtk::TreeModel::ColumnRecord
{
public:
  ModelColumns() {
	  add(m_col_name);
	  add(m_col_type);
	  add(m_col_uri);
	  add(m_col_label);
	  //add(m_col_library);
	  //add(m_col_label);
	  add(m_col_plugin_model);
  }

  Gtk::TreeModelColumn<Glib::ustring> m_col_name;
  Gtk::TreeModelColumn<Glib::ustring> m_col_type;
  Gtk::TreeModelColumn<Glib::ustring> m_col_uri;

  // Not displayed:
  Gtk::TreeModelColumn<Glib::ustring>            m_col_label;
  Gtk::TreeModelColumn<CountedPtr<PluginModel> > m_col_plugin_model;
};


/** Column for the criteria combo box in the load plugin window.
 *
 * \ingroup OmGtk
 */
class CriteriaColumns : public Gtk::TreeModel::ColumnRecord
{
public:
	enum Criteria { NAME, TYPE, URI, };
	
	CriteriaColumns() { add(m_col_label); add(m_col_criteria); }
	
	Gtk::TreeModelColumn<Glib::ustring> m_col_label;
	Gtk::TreeModelColumn<Criteria>      m_col_criteria;
};


/** 'Load Plugin' window.
 *
 * Loaded by glade as a derived object.
 *
 * \ingroup OmGtk
 */
class LoadPluginWindow : public Gtk::Window
{
public:
	LoadPluginWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);

	void patch_controller(PatchController* pc);
	void set_plugin_list(const std::map<string, CountedPtr<PluginModel> >& m);

	void set_next_module_location(int x, int y)
		{ m_new_module_x = x; m_new_module_y = y; }
	
	void add_plugin(CountedPtr<PluginModel> plugin);
	bool has_shown() const { return m_has_shown; }

protected:
	void on_show();
	void on_hide();
	bool on_key_press_event(GdkEventKey* event);
	
private:
	void add_clicked();
	void close_clicked();
	//void ok_clicked();
	void filter_changed();
	void clear_clicked();

	void plugin_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* col);
	void plugin_selection_changed();
	string generate_module_name(int offset = 0);

	PatchController* m_patch_controller;
	bool m_has_shown; // plugin list only populated on show to speed patch window creation

	Glib::RefPtr<Gtk::ListStore> m_plugins_liststore;
	ModelColumns                 m_plugins_columns;

	Glib::RefPtr<Gtk::ListStore> m_criteria_liststore;
	CriteriaColumns              m_criteria_columns;
	
	Glib::RefPtr<Gtk::TreeSelection> m_selection;
	
	int m_plugin_name_offset; // see comments for generate_plugin_name
	
	int m_new_module_x;
	int m_new_module_y;

	Gtk::TreeView*    m_plugins_treeview;
	Gtk::CheckButton* m_polyphonic_checkbutton;
	Gtk::Entry*       m_node_name_entry;
	Gtk::Button*      m_clear_button;
	Gtk::Button*      m_add_button;
	Gtk::Button*      m_close_button;
	//Gtk::Button*      m_ok_button;
	Gtk::ComboBox*    m_filter_combo;
	Gtk::Entry*       m_search_entry;
};


} // namespace OmGtk

#endif // LOADPLUGINWINDOW_H
