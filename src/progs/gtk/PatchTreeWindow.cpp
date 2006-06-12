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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "PatchTreeWindow.h"
#include "Controller.h"
#include "PatchController.h"
#include "PatchWindow.h"
#include "SubpatchModule.h"
#include "PatchModel.h"
#include "util/Path.h"

using Om::Path;

namespace OmGtk {
	

PatchTreeWindow::PatchTreeWindow(BaseObjectType* cobject,
	const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::Window(cobject),
  m_enable_signal(true)
{
	xml->get_widget_derived("patches_treeview", m_patches_treeview);
	
	m_patch_treestore = Gtk::TreeStore::create(m_patch_tree_columns);
	m_patches_treeview->set_window(this);
	m_patches_treeview->set_model(m_patch_treestore);
	Gtk::TreeViewColumn* name_col = Gtk::manage(new Gtk::TreeViewColumn(
		"Patch", m_patch_tree_columns.name_col));
	Gtk::TreeViewColumn* enabled_col = Gtk::manage(new Gtk::TreeViewColumn(
		"Run", m_patch_tree_columns.enabled_col));
	name_col->set_resizable(true);
	name_col->set_expand(true);

	m_patches_treeview->append_column(*name_col);
	m_patches_treeview->append_column(*enabled_col);
	Gtk::CellRendererToggle* enabled_renderer = dynamic_cast<Gtk::CellRendererToggle*>(
		m_patches_treeview->get_column_cell_renderer(1));
	enabled_renderer->property_activatable() = true;
	
	m_patch_tree_selection = m_patches_treeview->get_selection();
	
	//m_patch_tree_selection->signal_changed().connect(
	//	sigc::mem_fun(this, &PatchTreeWindow::event_patch_selected));   
	m_patches_treeview->signal_row_activated().connect(
		sigc::mem_fun(this, &PatchTreeWindow::event_patch_activated));
	enabled_renderer->signal_toggled().connect(
		sigc::mem_fun(this, &PatchTreeWindow::event_patch_enabled_toggled));
	
	m_patches_treeview->columns_autosize();
}


void
PatchTreeWindow::add_patch(PatchController* pc)
{
	const CountedPtr<PatchModel> pm = pc->patch_model();

	if (!pm->parent()) {
		Gtk::TreeModel::iterator iter = m_patch_treestore->append();
		Gtk::TreeModel::Row row = *iter;
		if (pm->path() == "/") {
			string root_name = Controller::instance().engine_url();
			// Hack off trailing '/' if it's there (ugly)
			//if (root_name.substr(root_name.length()-1,1) == "/")
			//	root_name = root_name.substr(0, root_name.length()-1);
			//root_name.append(":/");
			row[m_patch_tree_columns.name_col] = root_name;
		} else {
			row[m_patch_tree_columns.name_col] = pm->path().name();
		}
		row[m_patch_tree_columns.enabled_col] = false;
		row[m_patch_tree_columns.patch_controller_col] = pc;
		m_patches_treeview->expand_row(m_patch_treestore->get_path(iter), true);
	} else {
		Gtk::TreeModel::Children children = m_patch_treestore->children();
		Gtk::TreeModel::iterator c = find_patch(children, pm->parent()->path());
		
		if (c != children.end()) {
			Gtk::TreeModel::iterator iter = m_patch_treestore->append(c->children());
			Gtk::TreeModel::Row row = *iter;
			row[m_patch_tree_columns.name_col] = pm->path().name();
			row[m_patch_tree_columns.enabled_col] = false;
			row[m_patch_tree_columns.patch_controller_col] = pc;
			m_patches_treeview->expand_row(m_patch_treestore->get_path(iter), true);
		}
	}
}


void
PatchTreeWindow::remove_patch(const Path& path)
{
	Gtk::TreeModel::iterator i = find_patch(m_patch_treestore->children(), path);
	if (i != m_patch_treestore->children().end())
		m_patch_treestore->erase(i);
}


Gtk::TreeModel::iterator
PatchTreeWindow::find_patch(Gtk::TreeModel::Children root, const Path& path)
{
	PatchController* pc = NULL;
	
	for (Gtk::TreeModel::iterator c = root.begin(); c != root.end(); ++c) {
		pc = (*c)[m_patch_tree_columns.patch_controller_col];
		if (pc->model()->path() == path) {
			return c;
		} else if ((*c)->children().size() > 0) {
			Gtk::TreeModel::iterator ret = find_patch(c->children(), path);
			if (ret != c->children().end())
				return ret;
		}
	}
	return root.end();
}

/*
void
PatchTreeWindow::event_patch_selected()
{
	Gtk::TreeModel::iterator active = m_patch_tree_selection->get_selected();
	if (active) {
		Gtk::TreeModel::Row row = *active;
		PatchController* pc = row[m_patch_tree_columns.patch_controller_col];
	}
}
*/


/** Show the context menu for the selected patch in the patches treeview.
 */
void
PatchTreeWindow::show_patch_menu(GdkEventButton* ev)
{
	Gtk::TreeModel::iterator active = m_patch_tree_selection->get_selected();
	if (active) {
		Gtk::TreeModel::Row row = *active;
		PatchController* pc = row[m_patch_tree_columns.patch_controller_col];
		if (pc != NULL)
			pc->show_menu(ev);
	}
}


void
PatchTreeWindow::event_patch_activated(const Gtk::TreeModel::Path& path, Gtk::TreeView::Column* col)
{
	Gtk::TreeModel::iterator active = m_patch_treestore->get_iter(path);
	Gtk::TreeModel::Row row = *active;
	PatchController* pc = row[m_patch_tree_columns.patch_controller_col];
	
	pc->show_patch_window();
}


void
PatchTreeWindow::event_patch_enabled_toggled(const Glib::ustring& path_str)
{
	Gtk::TreeModel::Path path(path_str);
	Gtk::TreeModel::iterator active = m_patch_treestore->get_iter(path);
	Gtk::TreeModel::Row row = *active;
	
	PatchController* pc = row[m_patch_tree_columns.patch_controller_col];
	Glib::ustring patch_path = pc->model()->path();
	
	assert(pc != NULL);
	
	if ( ! pc->patch_model()->enabled()) {
		if (m_enable_signal)
			Controller::instance().enable_patch(patch_path);
		pc->enable();
		row[m_patch_tree_columns.enabled_col] = true;
	} else {
		if (m_enable_signal)
			Controller::instance().disable_patch(patch_path);
		pc->disable();
		row[m_patch_tree_columns.enabled_col] = false;
	}
}


void
PatchTreeWindow::patch_enabled(const Path& path)
{
	m_enable_signal = false;

	Gtk::TreeModel::iterator i
		= find_patch(m_patch_treestore->children(), path);
	
	if (i != m_patch_treestore->children().end()) {
		Gtk::TreeModel::Row row = *i;
		row[m_patch_tree_columns.enabled_col] = true;
	} else {
		cerr << "[PatchTreeWindow] Unable to find patch " << path << endl;
	}
	
	m_enable_signal = true;
}


void
PatchTreeWindow::patch_disabled(const Path& path)
{
	m_enable_signal = false;

	Gtk::TreeModel::iterator i
		= find_patch(m_patch_treestore->children(), path);
	
	if (i != m_patch_treestore->children().end()) {
		Gtk::TreeModel::Row row = *i;
		row[m_patch_tree_columns.enabled_col] = false;
	} else {
		cerr << "[PatchTreeWindow] Unable to find patch " << path << endl;
	}
	
	m_enable_signal = true;
}


void
PatchTreeWindow::patch_renamed(const Path& old_path, const Path& new_path)
{
	m_enable_signal = false;

	Gtk::TreeModel::iterator i
		= find_patch(m_patch_treestore->children(), old_path);
	
	if (i != m_patch_treestore->children().end()) {
		Gtk::TreeModel::Row row = *i;
		row[m_patch_tree_columns.name_col] = new_path.name();
	} else {
		cerr << "[PatchTreeWindow] Unable to find patch " << old_path << endl;
	}
	
	m_enable_signal = true;
}

	
} // namespace OmGtk
