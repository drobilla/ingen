/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_GUI_LOADSUBPATCHWINDOW_HPP
#define INGEN_GUI_LOADSUBPATCHWINDOW_HPP

#include <libglademm/xml.h>
#include <gtkmm.h>
#include "raul/SharedPtr.hpp"
#include "interface/GraphObject.hpp"
using namespace Ingen::Shared;

namespace Ingen {

namespace Client { class PatchModel; }
using Ingen::Client::PatchModel;

namespace GUI {


/** 'Add Subpatch' window.
 *
 * Loaded by glade as a derived object.
 *
 * \ingroup GUI
 */
class LoadPatchWindow : public Gtk::FileChooserDialog
{
public:
	LoadPatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);

	void set_patch(SharedPtr<PatchModel> patch);

	void present(SharedPtr<PatchModel> patch, bool import, GraphObject::Properties data);

protected:
	void on_show();

private:
	void merge_ports_selected();
	void insert_ports_selected();

	void selection_changed();
	void cancel_clicked();
	void ok_clicked();

	Raul::Symbol symbol_from_filename(const Glib::ustring& filename);
	Raul::Symbol avoid_symbol_clash(const Raul::Symbol& symbol);

	GraphObject::Properties _initial_data;

	SharedPtr<PatchModel> _patch;

	Gtk::Label*       _symbol_label;
	Gtk::Entry*       _symbol_entry;
	Gtk::Label*       _ports_label;
	Gtk::RadioButton* _merge_ports_radio;
	Gtk::RadioButton* _insert_ports_radio;
	Gtk::RadioButton* _poly_voices_radio;
	Gtk::RadioButton* _poly_from_file_radio;
	Gtk::SpinButton*  _poly_spinbutton;
	Gtk::Button*      _ok_button;
	Gtk::Button*      _cancel_button;

	bool _import;
	bool _merge_ports;
};


} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_LOADSUBPATCHWINDOW_HPP
