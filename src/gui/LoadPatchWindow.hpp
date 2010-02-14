/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef INGEN_GUI_LOADPATCHWINDOW_HPP
#define INGEN_GUI_LOADPATCHWINDOW_HPP

#include <libglademm/xml.h>
#include <gtkmm.h>
#include "raul/SharedPtr.hpp"
#include "interface/GraphObject.hpp"
#include "Window.hpp"

using namespace Ingen::Shared;

namespace Ingen {

namespace Client { class PatchModel; class PluginModel; }
using Ingen::Client::PluginModel;
using Ingen::Client::PatchModel;

namespace GUI {


/** 'Load Patch' window.
 *
 * Loaded by glade as a derived object.  Used for both "Import" and "Load"
 * (e.g. Replace, clear-then-import) operations (the radio button state
 * should be changed with the provided methods before presenting).
 *
 * This is not for loading subpatches.  See @a LoadSubpatchWindow for that.
 *
 * \ingroup GUI
 */
class LoadPatchWindow : public Gtk::FileChooserDialog
{
public:
	LoadPatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);

	void set_patch(SharedPtr<PatchModel> patch);

	void present(SharedPtr<PatchModel> patch, GraphObject::Properties data);

protected:
	void on_show();

private:
	void poly_voices_selected();
	void poly_from_file_selected();
	void merge_ports_selected();
	void insert_ports_selected();
	void ok_clicked();
	void cancel_clicked();

	GraphObject::Properties _initial_data;

	SharedPtr<PatchModel> _patch;
	bool                  _merge_ports;

	Gtk::RadioButton* _poly_voices_radio;
	Gtk::RadioButton* _poly_from_file_radio;
	Gtk::SpinButton*  _poly_spinbutton;
	Gtk::RadioButton* _merge_ports_radio;
	Gtk::RadioButton* _insert_ports_radio;
	Gtk::Button*      _ok_button;
	Gtk::Button*      _cancel_button;
};


} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_LOADPATCHWINDOW_HPP
