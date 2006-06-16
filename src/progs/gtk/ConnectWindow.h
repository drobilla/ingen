/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef CONNECT_WINDOW_H
#define CONNECT_WINDOW_H

#include <gtkmm.h>
#include <libglademm/xml.h>
#include <libglademm.h>
#include "util/CountedPtr.h"
#include "interface/ClientInterface.h"

namespace OmGtk {

class App;
class Controller;


/** The initially visible "Connect to engine" window.
 *
 * This handles actually connecting to the engine and making sure everything
 * is ready before really launching the app (eg wait for the root patch).
 *
 * \ingroup OmGtk
 */
class ConnectWindow : public Gtk::Dialog
{
public:
	ConnectWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);
	
	void start(CountedPtr<Om::Shared::ClientInterface> client);
private:
	void server_toggled();
	void launch_toggled();
	void internal_toggled();
	
	void disconnect();
	void connect();
	void quit();

	bool gtk_callback();

	CountedPtr<Om::Shared::ClientInterface> _client;
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


} // namespace OmGtk

#endif // CONNECT_WINDOW_H
