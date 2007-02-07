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

#include "App.h"
#include "ModelEngineInterface.h"
#include "OSCEngineSender.h"
#include "PatchTreeWindow.h"
#include "PatchWindow.h"
#include "Store.h"
#include "SubpatchModule.h"
#include "PatchModel.h"
#include "WindowFactory.h"
#include "raul/Path.h"

namespace Ingenuity {
	

PatchTreeWindow::PatchTreeWindow(BaseObjectType* cobject,
	const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::Window(cobject),
  _enable_signal(true)
{
	xml->get_widget_derived("patches_treeview", _patches_treeview);
	
	_patch_treestore = Gtk::TreeStore::create(_patch_tree_columns);
	_patches_treeview->set_window(this);
	_patches_treeview->set_model(_patch_treestore);
	Gtk::TreeViewColumn* name_col = Gtk::manage(new Gtk::TreeViewColumn(
		"Patch", _patch_tree_columns.name_col));
	Gtk::TreeViewColumn* enabled_col = Gtk::manage(new Gtk::TreeViewColumn(
		"Run", _patch_tree_columns.enabled_col));
	name_col->set_resizable(true);
	name_col->set_expand(true);

	_patches_treeview->append_column(*name_col);
	_patches_treeview->append_column(*enabled_col);
	Gtk::CellRendererToggle* enabled_renderer = dynamic_cast<Gtk::CellRendererToggle*>(
		_patches_treeview->get_column_cell_renderer(1));
	enabled_renderer->property_activatable() = true;
	
	_patch_tree_selection = _patches_treeview->get_selection();
	
	//m_patch_tree_selection->signal_changed().connect(
	//	sigc::mem_fun(this, &PatchTreeWindow::event_patch_selected));   
	_patches_treeview->signal_row_activated().connect(
		sigc::mem_fun(this, &PatchTreeWindow::event_patch_activated));
	enabled_renderer->signal_toggled().connect(
		sigc::mem_fun(this, &PatchTreeWindow::event_patch_enabled_toggled));
	
	_patches_treeview->columns_autosize();
}


void
PatchTreeWindow::init(Store& store)
{
	store.new_object_sig.connect(sigc::mem_fun(this, &PatchTreeWindow::new_object));
}


void
PatchTreeWindow::new_object(SharedPtr<ObjectModel> object)
{
	SharedPtr<PatchModel> patch = PtrCast<PatchModel>(object);
	if (patch)
		add_patch(patch);
}


void
PatchTreeWindow::add_patch(SharedPtr<PatchModel> pm)
{
	if (!pm->parent()) {
		Gtk::TreeModel::iterator iter = _patch_treestore->append();
		Gtk::TreeModel::Row row = *iter;
		if (pm->path() == "/") {
			SharedPtr<OSCEngineSender> osc_sender = PtrCast<OSCEngineSender>(App::instance().engine());
			string root_name = osc_sender ? osc_sender->engine_url() : "Internal";
			// Hack off trailing '/' if it's there (ugly)
			//if (root_name.substr(root_name.length()-1,1) == "/")
			//	root_name = root_name.substr(0, root_name.length()-1);
			//root_name.append(":/");
			row[_patch_tree_columns.name_col] = root_name;
		} else {
			row[_patch_tree_columns.name_col] = pm->path().name();
		}
		row[_patch_tree_columns.enabled_col] = false;
		row[_patch_tree_columns.patch_model_col] = pm;
		_patches_treeview->expand_row(_patch_treestore->get_path(iter), true);
	} else {
		Gtk::TreeModel::Children children = _patch_treestore->children();
		Gtk::TreeModel::iterator c = find_patch(children, pm->parent()->path());
		
		if (c != children.end()) {
			Gtk::TreeModel::iterator iter = _patch_treestore->append(c->children());
			Gtk::TreeModel::Row row = *iter;
			row[_patch_tree_columns.name_col] = pm->path().name();
			row[_patch_tree_columns.enabled_col] = false;
			row[_patch_tree_columns.patch_model_col] = pm;
			_patches_treeview->expand_row(_patch_treestore->get_path(iter), true);
		}
	}

	pm->enabled_sig.connect(sigc::bind(sigc::mem_fun(this, &PatchTreeWindow::patch_enabled), pm->path()));
	pm->disabled_sig.connect(sigc::bind(sigc::mem_fun(this, &PatchTreeWindow::patch_disabled), pm->path()));
}


void
PatchTreeWindow::remove_patch(const Path& path)
{
	Gtk::TreeModel::iterator i = find_patch(_patch_treestore->children(), path);
	if (i != _patch_treestore->children().end())
		_patch_treestore->erase(i);
}


Gtk::TreeModel::iterator
PatchTreeWindow::find_patch(Gtk::TreeModel::Children root, const Path& path)
{
	for (Gtk::TreeModel::iterator c = root.begin(); c != root.end(); ++c) {
		SharedPtr<PatchModel> pm = (*c)[_patch_tree_columns.patch_model_col];
		if (pm->path() == path) {
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
	Gtk::TreeModel::iterator active = _patch_tree_selection->get_selected();
	if (active) {
		Gtk::TreeModel::Row row = *active;
		SharedPtr<PatchModel> pm = row[_patch_tree_columns.patch_model_col];
	}
}
*/


/** Show the context menu for the selected patch in the patches treeview.
 */
void
PatchTreeWindow::show_patch_menu(GdkEventButton* ev)
{
	Gtk::TreeModel::iterator active = _patch_tree_selection->get_selected();
	if (active) {
		Gtk::TreeModel::Row row = *active;
		SharedPtr<PatchModel> pm = row[_patch_tree_columns.patch_model_col];
		if (pm)
			cerr << "FIXME: patch menu\n";
			//pm->show_menu(ev);
	}
}


void
PatchTreeWindow::event_patch_activated(const Gtk::TreeModel::Path& path, Gtk::TreeView::Column* col)
{
	Gtk::TreeModel::iterator active = _patch_treestore->get_iter(path);
	Gtk::TreeModel::Row row = *active;
	SharedPtr<PatchModel> pm = row[_patch_tree_columns.patch_model_col];
	
	App::instance().window_factory()->present_patch(pm);
}


void
PatchTreeWindow::event_patch_enabled_toggled(const Glib::ustring& path_str)
{
	Gtk::TreeModel::Path path(path_str);
	Gtk::TreeModel::iterator active = _patch_treestore->get_iter(path);
	Gtk::TreeModel::Row row = *active;
	
	SharedPtr<PatchModel> pm = row[_patch_tree_columns.patch_model_col];
	Glib::ustring patch_path = pm->path();
	
	assert(pm);
	
	if ( ! pm->enabled()) {
		if (_enable_signal)
			App::instance().engine()->enable_patch(patch_path);
		//row[_patch_tree_columns.enabled_col] = true;
	} else {
		if (_enable_signal)
			App::instance().engine()->disable_patch(patch_path);
		//row[_patch_tree_columns.enabled_col] = false;
	}
}


void
PatchTreeWindow::patch_enabled(const Path& path)
{
	_enable_signal = false;

	Gtk::TreeModel::iterator i
		= find_patch(_patch_treestore->children(), path);
	
	if (i != _patch_treestore->children().end()) {
		Gtk::TreeModel::Row row = *i;
		row[_patch_tree_columns.enabled_col] = true;
	} else {
		cerr << "[PatchTreeWindow] Unable to find patch " << path << endl;
	}
	
	_enable_signal = true;
}


void
PatchTreeWindow::patch_disabled(const Path& path)
{
	_enable_signal = false;

	Gtk::TreeModel::iterator i
		= find_patch(_patch_treestore->children(), path);
	
	if (i != _patch_treestore->children().end()) {
		Gtk::TreeModel::Row row = *i;
		row[_patch_tree_columns.enabled_col] = false;
	} else {
		cerr << "[PatchTreeWindow] Unable to find patch " << path << endl;
	}
	
	_enable_signal = true;
}


void
PatchTreeWindow::patch_renamed(const Path& old_path, const Path& new_path)
{
	_enable_signal = false;

	Gtk::TreeModel::iterator i
		= find_patch(_patch_treestore->children(), old_path);
	
	if (i != _patch_treestore->children().end()) {
		Gtk::TreeModel::Row row = *i;
		row[_patch_tree_columns.name_col] = new_path.name();
	} else {
		cerr << "[PatchTreeWindow] Unable to find patch " << old_path << endl;
	}
	
	_enable_signal = true;
}

	
} // namespace Ingenuity
