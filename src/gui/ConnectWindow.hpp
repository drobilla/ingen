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

#ifndef CONNECT_WINDOW_H
#define CONNECT_WINDOW_H

#include "config.h"

#ifdef HAVE_SLV2
#include <slv2/slv2.h>
#endif

#include <gtkmm.h>
#include <libglademm/xml.h>
#include <libglademm.h>
#include <raul/SharedPtr.hpp>
#include "client/ThreadedSigClientInterface.hpp"
using Ingen::Client::SigClientInterface;

namespace Ingen { class Engine; class QueuedEngineInterface; }

namespace Ingen {
namespace GUI {

class App;


/** The initially visible "Connect to engine" window.
 *
 * This handles actually connecting to the engine and making sure everything
 * is ready before really launching the app (eg wait for the root patch).
 *
 * \ingroup GUI
 */
class ConnectWindow : public Gtk::Dialog
{
public:
	ConnectWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);
	
	void set_connected_to(SharedPtr<Shared::EngineInterface> engine);
	void start(Ingen::Shared::World* world);
	void on_response(int32_t id) { _attached = true; }

private:
	enum Mode { CONNECT_REMOTE, LAUNCH_REMOTE, INTERNAL };

	void server_toggled();
	void launch_toggled();
	void internal_toggled();
	
	void disconnect();
	void connect(bool existing);
	void quit();
	void on_show();
	void on_hide();

	void load_widgets();
	void set_connecting_widget_states();

	bool gtk_callback();

	const Glib::RefPtr<Gnome::Glade::Xml> _xml;

	Mode    _mode;
	int32_t _ping_id;
	bool    _attached;
	
	bool _widgets_loaded;
	int  _connect_stage;

	SharedPtr<Glib::Module> _engine_module;
	SharedPtr<Glib::Module> _engine_jack_module;
	Ingen::Engine* (*_new_engine)(Ingen::Shared::World* world);

	Gtk::Image*        _icon;
	Gtk::ProgressBar*  _progress_bar;
	Gtk::Label*        _progress_label;
	Gtk::Entry*        _url_entry;
	Gtk::RadioButton*  _server_radio;
	Gtk::SpinButton*   _port_spinbutton;
	Gtk::RadioButton*  _launch_radio;
	Gtk::RadioButton*  _internal_radio;
	Gtk::Button*       _disconnect_button;
	Gtk::Button*       _connect_button;
	Gtk::Button*       _quit_button;
};


} // namespace GUI
} // namespace Ingen

#endif // CONNECT_WINDOW_H
