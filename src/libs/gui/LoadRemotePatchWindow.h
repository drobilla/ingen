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

#ifndef LOADREMOTEPATCHWINDOW_H
#define LOADREMOTEPATCHWINDOW_H

#include <libglademm/xml.h>
#include <gtkmm.h>
#include <raul/SharedPtr.h>
#include "client/PatchModel.h"
#include "client/PluginModel.h"
using Ingen::Client::PatchModel;
using Ingen::Client::MetadataMap;

namespace Ingen {
namespace GUI {


/** Columns for the remote patch list.
 *
 * \ingroup GUI
 */
class PatchColumns : public Gtk::TreeModel::ColumnRecord
{
public:
  PatchColumns() {
	  add(_col_name);
	  add(_col_uri);
  }

  Gtk::TreeModelColumn<Glib::ustring> _col_name;
  Gtk::TreeModelColumn<Glib::ustring> _col_uri;
};



/* Load remote patch ("import location") dialog.
 *
 * \ingroup GUI
 */
class LoadRemotePatchWindow : public Gtk::Dialog
{
public:
	LoadRemotePatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);

	void set_patch(SharedPtr<PatchModel> patch);

	void set_replace() { _replace = true; }
	void set_merge()   { _replace = false; }

	void present(SharedPtr<PatchModel> patch, MetadataMap data);

private:
	void patch_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* col);
	void patch_selected();
	void uri_changed();
	void open_clicked();
	void cancel_clicked();

	MetadataMap _initial_data;

	SharedPtr<PatchModel> _patch;
	bool                  _replace;
	
	Glib::RefPtr<Gtk::TreeSelection> _selection;
	Glib::RefPtr<Gtk::ListStore>     _liststore;
	PatchColumns                     _columns;

	Gtk::TreeView* _treeview;
	Gtk::Entry*    _uri_entry;
	Gtk::Button*   _open_button;
	Gtk::Button*   _cancel_button;
};
 

} // namespace GUI
} // namespace Ingen

#endif // LOADREMOTEPATCHWINDOW_H
