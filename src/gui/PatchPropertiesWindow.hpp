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

#ifndef PATCHPROPERTIESWINDOW_H
#define PATCHPROPERTIESWINDOW_H

#include <string>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include <raul/SharedPtr.hpp>
using std::string;

namespace Ingen { namespace Client { class PatchModel; } }
using Ingen::Client::PatchModel;

namespace Ingen {
namespace GUI {
	

/** Patch Properties Window.
 *
 * Loaded by libglade as a derived object.
 *
 * \ingroup GUI
 */
class PatchPropertiesWindow : public Gtk::Window
{
public:
	PatchPropertiesWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade);

	void present(SharedPtr<PatchModel> patch_model) { set_patch(patch_model); Gtk::Window::present(); }
	void set_patch(SharedPtr<PatchModel> patch_model);
	
	void cancel_clicked();
	void ok_clicked();

private:
	SharedPtr<PatchModel> _patch_model;

	Gtk::Entry*    _name_entry;
	Gtk::Entry*    _author_entry;
	Gtk::TextView* _textview;
	Gtk::Button*   _cancel_button;
	Gtk::Button*   _ok_button;
};


} // namespace GUI
} // namespace Ingen

#endif // PATCHPROPERTIESWINDOW_H
