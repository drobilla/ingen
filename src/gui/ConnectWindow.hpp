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

#ifndef INGEN_GUI_CONNECTWINDOW_HPP
#define INGEN_GUI_CONNECTWINDOW_HPP

#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/spinbutton.h>

#include "ingen/types.hpp"
#include "lilv/lilv.h"

#include "Window.hpp"

namespace ingen {
namespace gui {

class App;

/** The initially visible "Connect to engine" window.
 *
 * This handles actually connecting to the engine and making sure everything
 * is ready before really launching the app.
 *
 * \ingroup GUI
 */
class ConnectWindow : public Dialog
{
public:
	ConnectWindow(BaseObjectType*            cobject,
	              Glib::RefPtr<Gtk::Builder> xml);

	void set_connected_to(SPtr<ingen::Interface> engine);
	void start(App& app, ingen::World* world);

	bool attached()  const { return _finished_connecting; }
	bool quit_flag() const { return _quit_flag; }

private:
	enum class Mode { CONNECT_REMOTE, LAUNCH_REMOTE, INTERNAL };

	void message(const Message& msg);

	void error(const std::string& msg);

	void ingen_response(int32_t id, Status status, const std::string& subject);

	void server_toggled();
	void launch_toggled();
	void internal_toggled();

	void disconnect();
	void next_stage();
	bool connect_remote(const URI& uri);
	void connect(bool existing);
	void activate();
	void deactivate();
	void quit_clicked();
	void on_show() override;
	void on_hide() override;

	void load_widgets();
	void set_connecting_widget_states();

	bool gtk_callback();
	void quit();

	const Glib::RefPtr<Gtk::Builder> _xml;

	Gtk::Image*        _icon;
	Gtk::ProgressBar*  _progress_bar;
	Gtk::Label*        _progress_label;
	Gtk::Entry*        _url_entry;
	Gtk::RadioButton*  _server_radio;
	Gtk::SpinButton*   _port_spinbutton;
	Gtk::RadioButton*  _launch_radio;
	Gtk::RadioButton*  _internal_radio;
	Gtk::Button*       _activate_button;
	Gtk::Button*       _deactivate_button;
	Gtk::Button*       _disconnect_button;
	Gtk::Button*       _connect_button;
	Gtk::Button*       _quit_button;

	Mode      _mode;
	URI       _connect_uri;
	int32_t   _ping_id;
	bool      _attached;
	bool      _finished_connecting;
	bool      _widgets_loaded;
	int       _connect_stage;
	bool      _quit_flag;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_CONNECTWINDOW_HPP
