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

#include "Window.hpp"

#include <ingen/Message.hpp>
#include <ingen/URI.hpp>

#include <glibmm/refptr.h>
#include <gtkmm/builder.h>

#include <cstdint>
#include <memory>
#include <string>

namespace Gtk {
class Button;
class Entry;
class Image;
class Label;
class ProgressBar;
class RadioButton;
class SpinButton;
} // namespace Gtk

namespace ingen {

enum class Status;

class Interface;
class World;

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

	void set_connected_to(const std::shared_ptr<ingen::Interface>& engine);
	void start(App& app, ingen::World& world);

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

	Gtk::Image*        _icon{nullptr};
	Gtk::ProgressBar*  _progress_bar{nullptr};
	Gtk::Label*        _progress_label{nullptr};
	Gtk::Entry*        _url_entry{nullptr};
	Gtk::RadioButton*  _server_radio{nullptr};
	Gtk::SpinButton*   _port_spinbutton{nullptr};
	Gtk::RadioButton*  _launch_radio{nullptr};
	Gtk::RadioButton*  _internal_radio{nullptr};
	Gtk::Button*       _activate_button{nullptr};
	Gtk::Button*       _deactivate_button{nullptr};
	Gtk::Button*       _disconnect_button{nullptr};
	Gtk::Button*       _connect_button{nullptr};
	Gtk::Button*       _quit_button{nullptr};

	Mode      _mode{Mode::CONNECT_REMOTE};
	URI       _connect_uri{"unix:///tmp/ingen.sock"};
	int32_t   _ping_id{-1};
	bool      _attached{false};
	bool      _finished_connecting{false};
	bool      _widgets_loaded{false};
	int       _connect_stage{0};
	bool      _quit_flag{false};
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_CONNECTWINDOW_HPP
