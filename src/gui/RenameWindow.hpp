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

#ifndef RENAMEWINDOW_H
#define RENAMEWINDOW_H

#include <gtkmm.h>
#include <libglademm.h>
#include "raul/SharedPtr.hpp"
#include "client/ObjectModel.hpp"
using Ingen::Client::ObjectModel;

namespace Ingen {
namespace GUI {


/** Rename window.  Handles renaming of any (Ingen) object. 
 *
 * \ingroup GUI
 */
class RenameWindow : public Gtk::Window
{
public:
	RenameWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade);

	void present(SharedPtr<ObjectModel> object) { set_object(object); Gtk::Window::present(); }

private:
	void set_object(SharedPtr<ObjectModel> object);

	void name_changed();
	void cancel_clicked();
	void ok_clicked();
	
	SharedPtr<ObjectModel> _object;

	Gtk::Entry*      _name_entry;
	Gtk::Label*      _message_label;
	Gtk::Button*     _cancel_button;
	Gtk::Button*     _ok_button;
};

} // namespace GUI
} // namespace Ingen

#endif // RENAMEWINDOW_H
