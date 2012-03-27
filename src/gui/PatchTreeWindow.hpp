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

#ifndef INGEN_GUI_PATCHTREEWINDOW_HPP
#define INGEN_GUI_PATCHTREEWINDOW_HPP

#include <gtkmm.h>

#include "Window.hpp"

namespace Raul { class Path; }

namespace Ingen {

namespace Client { class ClientStore; }

namespace GUI {

class PatchWindow;
class PatchTreeView;

/** Window with a TreeView of all loaded patches.
 *
 * \ingroup GUI
 */
class PatchTreeWindow : public Window
{
public:
	PatchTreeWindow(BaseObjectType*                   cobject,
	                const Glib::RefPtr<Gtk::Builder>& xml);

	void init(App& app, Client::ClientStore& store);

	void new_object(SharedPtr<Client::ObjectModel> object);

	void patch_property_changed(const Raul::URI& key, const Raul::Atom& value,
			SharedPtr<Client::PatchModel> pm);

	void patch_moved(SharedPtr<Client::PatchModel> patch);

	void add_patch(SharedPtr<Client::PatchModel> pm);
	void remove_patch(SharedPtr<Client::PatchModel> pm);
	void show_patch_menu(GdkEventButton* ev);

protected:
	void event_patch_activated(const Gtk::TreeModel::Path& path, Gtk::TreeView::Column* col);
	void event_patch_enabled_toggled(const Glib::ustring& path_str);

	Gtk::TreeModel::iterator find_patch(
			Gtk::TreeModel::Children       root,
			SharedPtr<Client::ObjectModel> patch);

	PatchTreeView* _patches_treeview;

	struct PatchTreeModelColumns : public Gtk::TreeModel::ColumnRecord
	{
		PatchTreeModelColumns()
		{ add(name_col); add(enabled_col); add(patch_model_col); }

		Gtk::TreeModelColumn<Glib::ustring>                  name_col;
		Gtk::TreeModelColumn<bool>                           enabled_col;
		Gtk::TreeModelColumn<SharedPtr<Client::PatchModel> > patch_model_col;
	};

	App*                             _app;
	PatchTreeModelColumns            _patch_tree_columns;
	Glib::RefPtr<Gtk::TreeStore>     _patch_treestore;
	Glib::RefPtr<Gtk::TreeSelection> _patch_tree_selection;
	bool                             _enable_signal;
};

/** Derived TreeView class to support context menus for patches */
class PatchTreeView : public Gtk::TreeView
{
public:
	PatchTreeView(BaseObjectType*                   cobject,
	              const Glib::RefPtr<Gtk::Builder>& xml)
		: Gtk::TreeView(cobject)
		, _window(NULL)
	{}

	void set_window(PatchTreeWindow* win) { _window = win; }

	bool on_button_press_event(GdkEventButton* ev) {
		bool ret = Gtk::TreeView::on_button_press_event(ev);

		if ((ev->type == GDK_BUTTON_PRESS) && (ev->button == 3))
			_window->show_patch_menu(ev);

		return ret;
	}

private:
	PatchTreeWindow* _window;

}; // struct PatchTreeView

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_PATCHTREEWINDOW_HPP
