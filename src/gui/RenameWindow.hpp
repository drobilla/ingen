/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

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

#include "Window.hpp"

#include <gtkmm/window.h>

#include <memory>

namespace Glib {
template <class T> class RefPtr;
} // namespace Glib

namespace Gtk {
class Builder;
class Button;
class Entry;
class Label;
} // namespace Gtk

namespace ingen {

namespace client {
class ObjectModel;
} // namespace client

namespace gui {

/** Rename window.  Handles renaming of any (Ingen) object.
 *
 * \ingroup GUI
 */
class RenameWindow : public Window
{
public:
	RenameWindow(BaseObjectType*                   cobject,
	             const Glib::RefPtr<Gtk::Builder>& xml);

	void present(const std::shared_ptr<const client::ObjectModel>& object);

private:
	void set_object(const std::shared_ptr<const client::ObjectModel>& object);

	void values_changed();
	void cancel_clicked();
	void ok_clicked();

	std::shared_ptr<const client::ObjectModel> _object;

	Gtk::Entry*  _symbol_entry{nullptr};
	Gtk::Entry*  _label_entry{nullptr};
	Gtk::Label*  _message_label{nullptr};
	Gtk::Button* _cancel_button{nullptr};
	Gtk::Button* _ok_button{nullptr};
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_RENAMEWINDOW_HPP
