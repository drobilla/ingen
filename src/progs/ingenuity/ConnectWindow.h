/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#ifndef CONNECT_WINDOW_H
#define CONNECT_WINDOW_H

#include <gtkmm.h>
#include <libglademm/xml.h>
#include <libglademm.h>
#include "util/CountedPtr.h"
#include "interface/ClientInterface.h"

namespace Ingenuity {

class App;
class Controller;


/** The initially visible "Connect to engine" window.
 *
 * This handles actually connecting to the engine and making sure everything
 * is ready before really launching the app (eg wait for the root patch).
 *
 * \ingroup Ingenuity
 */
class ConnectWindow : public Gtk::Dialog
{
public:
	ConnectWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);
	
	void start(CountedPtr<Ingen::Shared::ClientInterface> client);
private:
	void server_toggled();
	void launch_toggled();
	void internal_toggled();
	
	void disconnect();
	void connect();
	void quit();

	bool gtk_callback();

	CountedPtr<Ingen::Shared::ClientInterface> _client;
	Gtk::Image*                             _icon;
	Gtk::ProgressBar*                       _progress_bar;
	Gtk::Label*                             _progress_label;
	Gtk::Entry*                             _url_entry;
	Gtk::RadioButton*                       _server_radio;
	Gtk::SpinButton*                        _port_spinbutton;
	Gtk::RadioButton*                       _launch_radio;
	Gtk::RadioButton*                       _internal_radio;
	Gtk::Button*                            _disconnect_button;
	Gtk::Button*                            _connect_button;
	Gtk::Button*                            _quit_button;
};


} // namespace Ingenuity

#endif // CONNECT_WINDOW_H
