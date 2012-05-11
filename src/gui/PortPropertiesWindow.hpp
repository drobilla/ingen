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

#ifndef INGEN_GUI_PORTPROPERTIESWINDOW_HPP
#define INGEN_GUI_PORTPROPERTIESWINDOW_HPP

#include <list>

#include <gtkmm.h>

#include "raul/SharedPtr.hpp"

#include "ingen/client/PortModel.hpp"

#include "Window.hpp"

namespace Ingen {
namespace GUI {

/** Port properties window.
 *
 * Loaded from XML as a derived object.
 *
 * \ingroup GUI
 */
class PortPropertiesWindow : public Window
{
public:
	PortPropertiesWindow(BaseObjectType*                   cobject,
	                     const Glib::RefPtr<Gtk::Builder>& xml);

	void present(SharedPtr<const Client::PortModel> port_model);

private:
	void property_changed(const Raul::URI& key, const Raul::Atom& value);
	void min_changed();
	void max_changed();

	void ok();
	void cancel();

	float _initial_min;
	float _initial_max;

	SharedPtr<const Client::PortModel> _port_model;
	Gtk::SpinButton*                   _min_spinner;
	Gtk::SpinButton*                   _max_spinner;
	Gtk::Button*                       _cancel_button;
	Gtk::Button*                       _ok_button;
	std::list<sigc::connection>        _connections;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_PORTPROPERTIESWINDOW_HPP
