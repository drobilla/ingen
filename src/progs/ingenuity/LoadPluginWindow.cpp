/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include "LoadPluginWindow.h"
#include <iostream>
#include <cassert>
#include <algorithm>
#include <cctype>
#include "PatchController.h"
#include "NodeModel.h"
#include "App.h"
#include "PatchWindow.h"
#include "OmFlowCanvas.h"
#include "PatchModel.h"
#include "Store.h"
#include "ModelEngineInterface.h"
#include "PatchView.h"
#include "OmFlowCanvas.h"
using std::cout; using std::cerr; using std::endl;


namespace Ingenuity {

LoadPluginWindow::LoadPluginWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::Window(cobject),
  m_has_shown(false),
  m_plugin_name_offset(0),
  m_new_module_x(0),
  m_new_module_y(0)
{
	xml->get_widget("load_plugin_plugins_treeview", m_plugins_treeview);
	xml->get_widget("load_plugin_polyphonic_checkbutton", m_polyphonic_checkbutton);
	xml->get_widget("load_plugin_name_entry", m_node_name_entry);
	xml->get_widget("load_plugin_clear_button", m_clear_button);
	xml->get_widget("load_plugin_add_button", m_add_button);
	xml->get_widget("load_plugin_close_button", m_close_button);
	//xml->get_widget("load_plugin_ok_button", m_add_button);
	
	xml->get_widget("load_plugin_filter_combo", m_filter_combo);
	xml->get_widget("load_plugin_search_entry", m_search_entry);
	
	// Set up the plugins list
	m_plugins_liststore = Gtk::ListStore::create(m_plugins_columns);
	m_plugins_treeview->set_model(m_plugins_liststore);
	m_plugins_treeview->append_column("Name", m_plugins_columns.m_col_name);
	m_plugins_treeview->append_column("Type", m_plugins_columns.m_col_type);
	m_plugins_treeview->append_column("URI", m_plugins_columns.m_col_uri);
	//m_plugins_treeview->append_column("Library", m_plugins_columns.m_col_library);
	//m_plugins_treeview->append_column("Label", m_plugins_columns.m_col_label);
		
	// This could be nicer.. store the TreeViewColumns locally maybe?
	m_plugins_treeview->get_column(0)->set_sort_column(m_plugins_columns.m_col_name);
	m_plugins_treeview->get_column(1)->set_sort_column(m_plugins_columns.m_col_type);
	m_plugins_treeview->get_column(2)->set_sort_column(m_plugins_columns.m_col_uri);
	//m_plugins_treeview->get_column(3)->set_sort_column(m_plugins_columns.m_col_library);
	//m_plugins_treeview->get_column(4)->set_sort_column(m_plugins_columns.m_col_label);
	for (int i=0; i < 3; ++i)
		m_plugins_treeview->get_column(i)->set_resizable(true);

	// Set up the search criteria combobox
	m_criteria_liststore = Gtk::ListStore::create(m_criteria_columns);
	m_filter_combo->set_model(m_criteria_liststore);
	Gtk::TreeModel::iterator iter = m_criteria_liststore->append();
	Gtk::TreeModel::Row row = *iter;
	row[m_criteria_columns.m_col_label] = "Name contains: ";
	row[m_criteria_columns.m_col_criteria] = CriteriaColumns::NAME;
	m_filter_combo->set_active(iter);
	iter = m_criteria_liststore->append(); row = *iter;
	row[m_criteria_columns.m_col_label] = "Type contains: ";
	row[m_criteria_columns.m_col_criteria] = CriteriaColumns::TYPE;
	iter = m_criteria_liststore->append(); row = *iter;
	row[m_criteria_columns.m_col_label] = "URI contains: ";
	row[m_criteria_columns.m_col_criteria] = CriteriaColumns::URI;
	/*iter = m_criteria_liststore->append(); row = *iter;
	row[m_criteria_columns.m_col_label] = "Library contains: ";
	row[m_criteria_columns.m_col_criteria] = CriteriaColumns::LIBRARY;
	iter = m_criteria_liststore->append(); row = *iter;
	row[m_criteria_columns.m_col_label] = "Label contains: ";
	row[m_criteria_columns.m_col_criteria] = CriteriaColumns::LABEL;*/

	m_clear_button->signal_clicked().connect(          sigc::mem_fun(this, &LoadPluginWindow::clear_clicked));
	m_add_button->signal_clicked().connect(            sigc::mem_fun(this, &LoadPluginWindow::add_clicked));
	m_close_button->signal_clicked().connect(          sigc::mem_fun(this, &LoadPluginWindow::close_clicked));
	//m_add_button->signal_clicked().connect(             sigc::mem_fun(this, &LoadPluginWindow::ok_clicked));
	m_plugins_treeview->signal_row_activated().connect(sigc::mem_fun(this, &LoadPluginWindow::plugin_activated));
	m_search_entry->signal_activate().connect(         sigc::mem_fun(this, &LoadPluginWindow::add_clicked));
	m_search_entry->signal_changed().connect(          sigc::mem_fun(this, &LoadPluginWindow::filter_changed));
	m_node_name_entry->signal_changed().connect(       sigc::mem_fun(this, &LoadPluginWindow::name_changed));

	m_selection = m_plugins_treeview->get_selection();
	m_selection->signal_changed().connect(sigc::mem_fun(this, &LoadPluginWindow::plugin_selection_changed));

	//m_add_button->grab_default();
}


/** Called every time the user types into the name input box.
 * Used to display warning messages, and enable/disable the OK button.
 */
void
LoadPluginWindow::name_changed()
{
	string name = m_node_name_entry->get_text();
	if (!Path::is_valid_name(name)) {
		//m_message_label->set_text("Name contains invalid characters.");
		m_add_button->property_sensitive() = false;
	} else if (m_patch_controller->patch_model()->get_node(name)) {
		//m_message_label->set_text("An object already exists with that name.");
		m_add_button->property_sensitive() = false;
	} else if (name.length() == 0) {
		//m_message_label->set_text("");
		m_add_button->property_sensitive() = false;
	} else {
		//m_message_label->set_text("");
		m_add_button->property_sensitive() = true;
	}	
}


/** Sets the patch controller for this window and initializes everything.
 *
 * This function MUST be called before using the window in any way!
 */
void
LoadPluginWindow::set_patch(CountedPtr<PatchController> pc)
{
	m_patch_controller = pc;

	if (pc->patch_model()->poly() <= 1)
		m_polyphonic_checkbutton->property_sensitive() = false;
	else
		m_polyphonic_checkbutton->property_sensitive() = true;
		
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
	if (!m_has_shown) {
		set_plugin_list(App::instance().store()->plugins());
	
		// Center on patch window
		int m_w, m_h;
		get_size(m_w, m_h);
		
		int parent_x, parent_y, parent_w, parent_h;
		m_patch_controller->window()->get_position(parent_x, parent_y);
		m_patch_controller->window()->get_size(parent_w, parent_h);
		
		move(parent_x + parent_w/2 - m_w/2, parent_y + parent_h/2 - m_h/2);
		
		m_has_shown = true;
	}
	Gtk::Window::on_show();
}


void
LoadPluginWindow::on_hide()
{
	m_new_module_x = 0;
	m_new_module_y = 0;
	Gtk::Window::on_hide();
}


void
LoadPluginWindow::set_plugin_list(const std::map<string, CountedPtr<PluginModel> >& m)
{
	m_plugins_liststore->clear();

	for (std::map<string, CountedPtr<PluginModel> >::const_iterator i = m.begin(); i != m.end(); ++i) {
		CountedPtr<PluginModel> plugin = (*i).second;

		Gtk::TreeModel::iterator iter = m_plugins_liststore->append();
		Gtk::TreeModel::Row row = *iter;
		
		row[m_plugins_columns.m_col_name] = plugin->name();
		//row[m_plugins_columns.m_col_label] = plugin->plug_label();
		row[m_plugins_columns.m_col_type] = plugin->type_string();
		row[m_plugins_columns.m_col_uri] = plugin->uri();
		row[m_plugins_columns.m_col_label] = plugin->name();
		//row[m_plugins_columns.m_col_library] = plugin->lib_name();
		row[m_plugins_columns.m_col_plugin_model] = plugin;
	}

	m_plugins_treeview->columns_autosize();
}


void
LoadPluginWindow::add_plugin(CountedPtr<PluginModel> plugin)
{
	Gtk::TreeModel::iterator iter = m_plugins_liststore->append();
	Gtk::TreeModel::Row row = *iter;
	
	row[m_plugins_columns.m_col_name] = plugin->name();
	//row[m_plugins_columns.m_col_label] = plugin->plug_label();
	row[m_plugins_columns.m_col_type] = plugin->type_string();
	row[m_plugins_columns.m_col_uri] = plugin->uri();
	row[m_plugins_columns.m_col_label] = plugin->name();
	//row[m_plugins_columns.m_col_library] = plugin->lib_name();
	row[m_plugins_columns.m_col_plugin_model] = plugin;
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
	m_plugin_name_offset = 0;

	m_node_name_entry->set_text(generate_module_name());
	
	//Gtk::TreeModel::iterator iter = m_selection->get_selected();
	//Gtk::TreeModel::Row row = *iter;
	//const PluginModel* plugin = row.get_value(m_plugins_columns.m_col_plugin_model);
}


/** Generate an automatic name for this Node.
 *
 * Offset is an offset of the number that will be appended to the plugin's
 * label, needed if the user adds multiple plugins faster than the engine
 * sends the notification back.
 */
string
LoadPluginWindow::generate_module_name(int offset)
{
	string name = "";

	Gtk::TreeModel::iterator iter = m_selection->get_selected();
	
	if (iter) {
		Gtk::TreeModel::Row row = *iter;
		CountedPtr<PluginModel> plugin = row.get_value(m_plugins_columns.m_col_plugin_model);
		char num_buf[3];
		for (uint i=0; i < 99; ++i) {
			name = plugin->plug_label();
			if (name == "")
				name = plugin->name().substr(0, plugin->name().find(' '));
			if (i+offset != 0) {
				snprintf(num_buf, 3, "%d", i+offset+1);
				name += "_";
				name += num_buf;
			}
			if (!m_patch_controller->patch_model()->get_node(name))
				break;
			else
				name = "";
		}
	}

	return name;
}


void
LoadPluginWindow::add_clicked()
{
	Gtk::TreeModel::iterator iter = m_selection->get_selected();
	bool polyphonic = m_polyphonic_checkbutton->get_active();
	
	if (iter) { // If anything is selected			
		Gtk::TreeModel::Row row = *iter;
		CountedPtr<PluginModel> plugin = row.get_value(m_plugins_columns.m_col_plugin_model);
		string name = m_node_name_entry->get_text();
		if (name == "") {
			name = generate_module_name();
		}
		if (name == "") {
			Gtk::MessageDialog dialog(*this,
				"Unable to chose a default name for this node.  Please enter a name.",
				false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);

			dialog.run();
		} else {
			const string path = m_patch_controller->model()->path().base() + name;
			NodeModel* nm = new NodeModel(plugin, path);
			nm->polyphonic(polyphonic);
			
			if (m_new_module_x == 0 && m_new_module_y == 0) {
				m_patch_controller->get_view()->canvas()->get_new_module_location(
					m_new_module_x, m_new_module_y);
			}
			nm->x(m_new_module_x);
			nm->y(m_new_module_y);

			App::instance().engine()->create_node_from_model(nm);
			++m_plugin_name_offset;
			m_node_name_entry->set_text(generate_module_name(m_plugin_name_offset));
			
			// Set the next module location 20 over, for a cascade effect
			m_new_module_x += 20;
			m_new_module_y += 20;
		}
	}
}


void
LoadPluginWindow::close_clicked()
{
	hide();
}


/*
void
LoadPluginWindow::ok_clicked()
{
	add_clicked();
	close_clicked();
}
*/

void
LoadPluginWindow::filter_changed()
{
	m_plugins_liststore->clear();

	string search = m_search_entry->get_text();
	transform(search.begin(), search.end(), search.begin(), toupper);

	// Get selected criteria
	const Gtk::TreeModel::Row row = *(m_filter_combo->get_active());
	CriteriaColumns::Criteria criteria = row[m_criteria_columns.m_col_criteria];
	
	string field;
	
	Gtk::TreeModel::Row      model_row;
	Gtk::TreeModel::iterator model_iter;
	size_t                   num_visible = 0;
	

	for (std::map<string, CountedPtr<PluginModel> >::const_iterator i = App::instance().store()->plugins().begin();
			i != App::instance().store()->plugins().end(); ++i) {
	
		const CountedPtr<PluginModel> plugin = (*i).second;

		switch (criteria) {
		case CriteriaColumns::NAME:
			field = plugin->name(); break;
		case CriteriaColumns::TYPE:
			field = plugin->type_string(); break;
		case CriteriaColumns::URI:
			field = plugin->uri(); break;
		/*case CriteriaColumns::LIBRARY:
			field = plugin->lib_name(); break;
		case CriteriaColumns::LABEL:
			field = plugin->plug_label(); break;*/
		default:
			throw;
		}
		
		transform(field.begin(), field.end(), field.begin(), toupper);
		
		if (field.find(search) != string::npos) {
			model_iter = m_plugins_liststore->append();
			model_row = *model_iter;
		
			model_row[m_plugins_columns.m_col_name] = plugin->name();
			//model_row[m_plugins_columns.m_col_label] = plugin->plug_label();
			model_row[m_plugins_columns.m_col_type] = plugin->type_string();
			model_row[m_plugins_columns.m_col_uri] = plugin->uri();
			model_row[m_plugins_columns.m_col_label] = plugin->plug_label();
			//model_row[m_plugins_columns.m_col_library] = plugin->lib_name();
			model_row[m_plugins_columns.m_col_plugin_model] = plugin;

			++num_visible;
		}
	}

	if (num_visible == 1) {
		m_selection->unselect_all();
		m_selection->select(model_iter);
	}
}


void
LoadPluginWindow::clear_clicked()
{
	m_search_entry->set_text("");
	set_plugin_list(App::instance().store()->plugins());
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


} // namespace Ingenuity
