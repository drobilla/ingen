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

#ifndef LOADSUBPATCHWINDOW_H
#define LOADSUBPATCHWINDOW_H

#include <libglademm/xml.h>
#include <gtkmm.h>
#include <raul/SharedPtr.hpp>
#include "client/PatchModel.hpp"
#include "client/PluginModel.hpp"
using Ingen::Client::PatchModel;
using Ingen::Client::MetadataMap;

namespace Ingen {
namespace GUI {
	

/** 'Add Subpatch' window.
 *
 * Loaded by glade as a derived object.
 *
 * \ingroup GUI
 */
class LoadSubpatchWindow : public Gtk::FileChooserDialog
{
public:
	LoadSubpatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);

	void set_patch(SharedPtr<PatchModel> patch);
	
	void present(SharedPtr<PatchModel> patch, MetadataMap data);

protected:
	void on_show();

private:
	void disable_name_entry();
	void enable_name_entry();
	void disable_poly_spinner();
	void enable_poly_spinner();
	
	void ok_clicked();
	void cancel_clicked();

	MetadataMap _initial_data;

	SharedPtr<PatchModel> _patch;
	
	Gtk::RadioButton* _name_from_file_radio;
	Gtk::RadioButton* _name_from_user_radio;
	Gtk::Entry*       _name_entry;
	Gtk::RadioButton* _poly_from_file_radio;
	Gtk::RadioButton* _poly_from_parent_radio;
	Gtk::RadioButton* _poly_from_user_radio;
	Gtk::SpinButton*  _poly_spinbutton;
	Gtk::Button*      _ok_button;
	Gtk::Button*      _cancel_button;
};
 

} // namespace GUI
} // namespace Ingen

#endif // LOADSUBPATCHWINDOW_H
