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

#ifndef PORTPROPERTIESWINDOW_H
#define PORTPROPERTIESWINDOW_H

#include <gtkmm.h>
#include <libglademm.h>
#include <raul/SharedPtr.hpp>
#include "client/PortModel.hpp"
using namespace Ingen::Client;

namespace Ingen {
namespace GUI {


/** Port properties window.
 *
 * Loaded by libglade as a derived object.
 *
 * \ingroup GUI
 */
class PortPropertiesWindow : public Gtk::Window
{
public:
	PortPropertiesWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade);

	void present(SharedPtr<PortModel> port_model);

private:
	void metadata_update(const string& key, const Atom& value);
	void min_changed();
	void max_changed();
	
	void ok();
	void cancel();

	bool _enable_signal;

	float _initial_min;
	float _initial_max;

	SharedPtr<PortModel>        _port_model;
	Gtk::SpinButton*            _min_spinner;
	Gtk::SpinButton*            _max_spinner;
	Gtk::Button*                _cancel_button;
	Gtk::Button*                _ok_button;
	std::list<sigc::connection> _connections;
};

} // namespace GUI
} // namespace Ingen

#endif // PORTPROPERTIESWINDOW_H
