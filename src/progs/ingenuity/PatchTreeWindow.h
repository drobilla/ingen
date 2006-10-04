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

#ifndef PATCHTREEWINDOW_H
#define PATCHTREEWINDOW_H

#include <gtkmm.h>
#include <libglademm.h>
#include "raul/Path.h"

namespace Ingen { namespace Client {
	class Store;
} }
using Ingen::Client::Store;

namespace Ingenuity {

class PatchWindow;
class PatchTreeView;


/** Window with a TreeView of all loaded patches.
 *
 * \ingroup Ingenuity
 */
class PatchTreeWindow : public Gtk::Window
{
public:
	PatchTreeWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade);

	void init(Store& store);

	void new_object(SharedPtr<ObjectModel> object);

	void patch_enabled(const Path& path);
	void patch_disabled(const Path& path);
	void patch_renamed(const Path& old_path, const Path& new_path);

	void add_patch(SharedPtr<PatchModel> pm);
	void remove_patch(const Path& path);
	void show_patch_menu(GdkEventButton* ev);

protected:
	//void event_patch_selected();
	void event_patch_activated(const Gtk::TreeModel::Path& path, Gtk::TreeView::Column* col);
	void event_patch_enabled_toggled(const Glib::ustring& path_str);

	Gtk::TreeModel::iterator find_patch(Gtk::TreeModel::Children root, const Path& path);
	
	PatchTreeView* m_patches_treeview;

	struct PatchTreeModelColumns : public Gtk::TreeModel::ColumnRecord
	{
		PatchTreeModelColumns()
		{ add(name_col); add(enabled_col); add(patch_model_col); }
		
		Gtk::TreeModelColumn<Glib::ustring>                name_col;
		Gtk::TreeModelColumn<bool>                         enabled_col;
		Gtk::TreeModelColumn<SharedPtr<PatchModel> > patch_model_col;
	};

	bool                             m_enable_signal;
	PatchTreeModelColumns            m_patch_tree_columns;
	Glib::RefPtr<Gtk::TreeStore>     m_patch_treestore;
	Glib::RefPtr<Gtk::TreeSelection> m_patch_tree_selection;
};


/** Derived TreeView class to support context menus for patches */
class PatchTreeView : public Gtk::TreeView
{
public:
	PatchTreeView(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Gtk::TreeView(cobject)
	{}

	void set_window(PatchTreeWindow* win) { m_window = win; }
	
	bool on_button_press_event(GdkEventButton* ev) {
		bool ret = Gtk::TreeView::on_button_press_event(ev);
	
		if ((ev->type == GDK_BUTTON_PRESS) && (ev->button == 3))
			m_window->show_patch_menu(ev);

		return ret;
	}
	
private:
	PatchTreeWindow* m_window;

}; // struct PatchTreeView

	
} // namespace Ingenuity

#endif // PATCHTREEWINDOW_H
