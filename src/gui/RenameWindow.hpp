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

#ifndef INGEN_GUI_RENAMEWINDOW_HPP
#define INGEN_GUI_RENAMEWINDOW_HPP

#include <gtkmm.h>

#include "raul/SharedPtr.hpp"

#include "ingen/client/ObjectModel.hpp"

#include "Window.hpp"

namespace Ingen {
namespace GUI {

/** Rename window.  Handles renaming of any (Ingen) object.
 *
 * \ingroup GUI
 */
class RenameWindow : public Window
{
public:
	RenameWindow(BaseObjectType*                   cobject,
	             const Glib::RefPtr<Gtk::Builder>& xml);

	void present(SharedPtr<const Client::ObjectModel> object);

private:
	void set_object(SharedPtr<const Client::ObjectModel> object);

	void values_changed();
	void cancel_clicked();
	void ok_clicked();

	SharedPtr<const Client::ObjectModel> _object;

	Gtk::Entry*  _symbol_entry;
	Gtk::Entry*  _label_entry;
	Gtk::Label*  _message_label;
	Gtk::Button* _cancel_button;
	Gtk::Button* _ok_button;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_RENAMEWINDOW_HPP
