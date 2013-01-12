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

#include <stdlib.h>
#include <string>
#include <sstream>

#include <gtkmm/stock.h>

#include "raul/Process.hpp"

#include "ingen/EngineBase.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/Module.hpp"
#include "ingen/World.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/GraphModel.hpp"
#include "ingen/client/ThreadedSigClientInterface.hpp"
#include "ingen_config.h"

#include "App.hpp"
#include "ConnectWindow.hpp"
#include "WindowFactory.hpp"

using namespace Ingen::Client;
using namespace std;

namespace Ingen {
namespace GUI {

ConnectWindow::ConnectWindow(BaseObjectType*                   cobject,
                             const Glib::RefPtr<Gtk::Builder>& xml)
	: Dialog(cobject)
	, _xml(xml)
	, _icon(NULL)
	, _progress_bar(NULL)
	, _progress_label(NULL)
	, _url_entry(NULL)
	, _server_radio(NULL)
	, _port_spinbutton(NULL)
	, _launch_radio(NULL)
	, _internal_radio(NULL)
	, _activate_button(NULL)
	, _deactivate_button(NULL)
	, _disconnect_button(NULL)
	, _connect_button(NULL)
	, _quit_button(NULL)
	, _mode(Mode::CONNECT_REMOTE)
	, _ping_id(-1)
	, _attached(false)
	, _finished_connecting(false)
	, _widgets_loaded(false)
	, _connect_stage(0)
	, _quit_flag(false)
{
}

void
ConnectWindow::start(App& app, Ingen::World* world)
{
	_app = &app;

	if (world->engine()) {
		_mode = Mode::INTERNAL;
		if (_widgets_loaded) {
			_internal_radio->set_active(true);
		}
	}

	set_connected_to(world->interface());
	connect(bool(world->interface()));
}

void
ConnectWindow::set_connected_to(SPtr<Ingen::Interface> engine)
{
	_app->world()->set_interface(engine);

	if (!_widgets_loaded)
		return;

	if (engine) {
		_icon->set(Gtk::Stock::CONNECT, Gtk::ICON_SIZE_LARGE_TOOLBAR);
		_progress_bar->set_fraction(1.0);
		_progress_label->set_text("Connected to engine");
		_url_entry->set_sensitive(false);
		_connect_button->set_sensitive(false);
		_disconnect_button->set_label("gtk-disconnect");
		_disconnect_button->set_sensitive(true);
		_port_spinbutton->set_sensitive(false);
		_launch_radio->set_sensitive(false);
		_internal_radio->set_sensitive(false);
	} else {
		_icon->set(Gtk::Stock::DISCONNECT, Gtk::ICON_SIZE_LARGE_TOOLBAR);
		_progress_bar->set_fraction(0.0);
		_connect_button->set_sensitive(true);
		_disconnect_button->set_sensitive(false);

		if (_app->world()->engine())
			_internal_radio->set_sensitive(true);
		else
			_internal_radio->set_sensitive(false);

		_server_radio->set_sensitive(true);
		_launch_radio->set_sensitive(true);

		if (_mode == Mode::CONNECT_REMOTE)
			_url_entry->set_sensitive(true);
		else if (_mode == Mode::LAUNCH_REMOTE)
			_port_spinbutton->set_sensitive(true);

		_progress_label->set_text(string("Disconnected"));
	}
}

void
ConnectWindow::set_connecting_widget_states()
{
	if (!_widgets_loaded)
		return;

	_connect_button->set_sensitive(false);
	_disconnect_button->set_label("gtk-cancel");
	_disconnect_button->set_sensitive(true);
	_server_radio->set_sensitive(false);
	_launch_radio->set_sensitive(false);
	_internal_radio->set_sensitive(false);
	_url_entry->set_sensitive(false);
	_port_spinbutton->set_sensitive(false);
}

/** Launch (if applicable) and connect to the Engine.
 *
 * This will create the required interfaces and initialize the App with them.
 */
void
ConnectWindow::connect(bool existing)
{
	if (_attached)
		_attached = false;

	assert(!_app->client());

	_connect_stage = 0;
	set_connecting_widget_states();

	Ingen::World* world = _app->world();

#ifdef HAVE_SOCKET
	if (_mode == Mode::CONNECT_REMOTE) {
		Raul::URI uri("unix:///tmp/ingen.sock");
		if (_widgets_loaded) {
			const std::string& user_uri = _url_entry->get_text();
			if (Raul::URI::is_valid(user_uri)) {
				uri = Raul::URI(user_uri);
			}
		}

		if (existing) {
			uri = world->interface()->uri();
		}

		SPtr<ThreadedSigClientInterface> tsci;
		if (world->interface()) {
			tsci = dynamic_ptr_cast<ThreadedSigClientInterface>(
				world->interface()->respondee());
		}

		if (!tsci) {
			world->set_interface(world->new_interface(uri, tsci));
		}

		_app->attach(tsci);
		_app->register_callbacks();

		Glib::signal_timeout().connect(
			sigc::mem_fun(this, &ConnectWindow::gtk_callback), 40);
		return;
	} else if (_mode == Mode::LAUNCH_REMOTE) {
		int port = _port_spinbutton->get_value_as_int();
		std::stringstream ss;
		ss << port;
		const std::string port_str = ss.str();
		const string cmd = string("ingen -e -E ").append(port_str);

		if (Raul::Process::launch(cmd)) {
			const Raul::URI engine_uri(string("tcp://localhost:") + port_str);

			SPtr<ThreadedSigClientInterface> tsci(new ThreadedSigClientInterface(1024));
			world->set_interface(world->new_interface(engine_uri, tsci));

			_app->attach(tsci);
			_app->register_callbacks();

			Glib::signal_timeout().connect(
				sigc::mem_fun(this, &ConnectWindow::gtk_callback), 40);
		} else {
			world->log().error("Failed to launch ingen process\n");
		}
		return;
	}
#endif
	if (_mode == Mode::INTERNAL) {
		if (!world->engine()) {
			world->load_module("server");
			world->load_module("jack");
			world->engine()->activate();
		}

		SPtr<SigClientInterface> client(new SigClientInterface());

		world->interface()->set_respondee(client);
		_app->attach(client);
		_app->register_callbacks();

		Glib::signal_timeout().connect(
			sigc::mem_fun(this, &ConnectWindow::gtk_callback), 10);
	}
}

void
ConnectWindow::disconnect()
{
	_connect_stage = -1;
	_attached = false;

	_app->detach();
	set_connected_to(SPtr<Ingen::Interface>());

	if (!_widgets_loaded)
		return;

	_activate_button->set_sensitive(false);
	_deactivate_button->set_sensitive(false);

	_progress_bar->set_fraction(0.0);
	_connect_button->set_sensitive(true);
	_disconnect_button->set_sensitive(false);
}

void
ConnectWindow::activate()
{
	_app->interface()->set_property(Raul::URI("ingen:/driver"),
	                                _app->uris().ingen_enabled,
	                                _app->forge().make(true));
}

void
ConnectWindow::deactivate()
{
	_app->interface()->set_property(Raul::URI("ingen:/driver"),
	                                _app->uris().ingen_enabled,
	                                _app->forge().make(false));
}

void
ConnectWindow::on_show()
{
	if (!_widgets_loaded) {
		load_widgets();
		if (_attached)
			set_connected_to(_app->interface());
	}

	Gtk::Dialog::on_show();
}

void
ConnectWindow::load_widgets()
{
	_xml->get_widget("connect_icon",                 _icon);
	_xml->get_widget("connect_progress_bar",         _progress_bar);
	_xml->get_widget("connect_progress_label",       _progress_label);
	_xml->get_widget("connect_server_radiobutton",   _server_radio);
	_xml->get_widget("connect_url_entry",            _url_entry);
	_xml->get_widget("connect_launch_radiobutton",   _launch_radio);
	_xml->get_widget("connect_port_spinbutton",      _port_spinbutton);
	_xml->get_widget("connect_internal_radiobutton", _internal_radio);
	_xml->get_widget("connect_activate_button",      _activate_button);
	_xml->get_widget("connect_deactivate_button",    _deactivate_button);
	_xml->get_widget("connect_disconnect_button",    _disconnect_button);
	_xml->get_widget("connect_connect_button",       _connect_button);
	_xml->get_widget("connect_quit_button",          _quit_button);

	_server_radio->signal_toggled().connect(
		sigc::mem_fun(this, &ConnectWindow::server_toggled));
	_launch_radio->signal_toggled().connect(
		sigc::mem_fun(this, &ConnectWindow::launch_toggled));
	_internal_radio->signal_clicked().connect(
		sigc::mem_fun(this, &ConnectWindow::internal_toggled));
	_activate_button->signal_clicked().connect(
		sigc::mem_fun(this, &ConnectWindow::activate));
	_deactivate_button->signal_clicked().connect(
		sigc::mem_fun(this, &ConnectWindow::deactivate));
	_disconnect_button->signal_clicked().connect(
		sigc::mem_fun(this, &ConnectWindow::disconnect));
	_connect_button->signal_clicked().connect(
		sigc::bind(sigc::mem_fun(this, &ConnectWindow::connect), false));
	_quit_button->signal_clicked().connect(
		sigc::mem_fun(this, &ConnectWindow::quit_clicked));

	_progress_bar->set_pulse_step(0.01);
	_widgets_loaded = true;

	server_toggled();
}

void
ConnectWindow::on_hide()
{
	Gtk::Dialog::on_hide();
	if (_app->window_factory()->num_open_graph_windows() == 0)
		quit();
}

void
ConnectWindow::quit_clicked()
{
	if (_app->quit(this))
		_quit_flag = true;
}

void
ConnectWindow::server_toggled()
{
	_url_entry->set_sensitive(true);
	_port_spinbutton->set_sensitive(false);
	_mode = Mode::CONNECT_REMOTE;
}

void
ConnectWindow::launch_toggled()
{
	_url_entry->set_sensitive(false);
	_port_spinbutton->set_sensitive(true);
	_mode = Mode::LAUNCH_REMOTE;
}

void
ConnectWindow::internal_toggled()
{
	_url_entry->set_sensitive(false);
	_port_spinbutton->set_sensitive(false);
	_mode = Mode::INTERNAL;
}

bool
ConnectWindow::gtk_callback()
{
	/* If I call this a "state machine" it's not ugly code any more */

	if (_quit_flag)
		return false; // deregister this callback

	// Timing stuff for repeated attach attempts
	timeval now;
	gettimeofday(&now, NULL);
	static const timeval start = now;
	static timeval       last  = now;

	// Show if attempted connection goes on for a noticeable amount of time
	if (!is_visible()) {
		const float ms_since_start = (now.tv_sec - start.tv_sec) * 1000.0f +
			(now.tv_usec - start.tv_usec) * 0.001f;
		if (ms_since_start > 500) {
			present();
			set_connecting_widget_states();
		}
	}

	if (_connect_stage == 0) {
		_attached = false;
		_app->client()->signal_response().connect(
			sigc::mem_fun(this, &ConnectWindow::ingen_response));

		_ping_id = g_random_int();
		_app->interface()->set_response_id(_ping_id);
		_app->interface()->get(Raul::URI("ingen:/engine"));

		if (_widgets_loaded) {
			_progress_label->set_text("Connecting to engine...");
			_progress_bar->set_pulse_step(0.01);
		}

		++_connect_stage;

	} else if (_connect_stage == 1) {
		if (_attached) {
			++_connect_stage;
		} else {
			const float ms_since_last = (now.tv_sec - last.tv_sec) * 1000.0f +
				(now.tv_usec - last.tv_usec) * 0.001f;
			if (ms_since_last > 1000) {
				_app->interface()->set_response_id(_ping_id);
				_app->interface()->get(Raul::URI("ingen:/engine"));
				last = now;
			}
		}
	} else if (_connect_stage == 2) {
		_app->interface()->get(Raul::URI(Node::root_uri() + "/"));
		if (_widgets_loaded)
			_progress_label->set_text(string("Requesting root graph..."));
		++_connect_stage;
	} else if (_connect_stage == 3) {
		if (_app->store()->size() > 0) {
			SPtr<const GraphModel> root = dynamic_ptr_cast<const GraphModel>(
				_app->store()->object(Raul::Path("/")));
			if (root) {
				set_connected_to(_app->interface());
				_app->window_factory()->present_graph(root);
				_app->interface()->get(Raul::URI("ingen:/plugins"));
				if (_widgets_loaded)
					_progress_label->set_text(string("Loading plugins..."));
				++_connect_stage;
			}
		}
	} else if (_connect_stage == 4) {
		_app->interface()->get(Raul::URI("ingen:/plugins"));
		hide();
		if (_widgets_loaded)
			_progress_label->set_text("Connected to engine");
		_connect_stage = 0; // set ourselves up for next time (if there is one)
		_finished_connecting = true;
		return false; // deregister this callback
	}

	if (_widgets_loaded)
		_progress_bar->pulse();

	if (_connect_stage == -1) { // we were cancelled
		if (_widgets_loaded) {
			_icon->set(Gtk::Stock::DISCONNECT, Gtk::ICON_SIZE_LARGE_TOOLBAR);
			_progress_bar->set_fraction(0.0);
			_connect_button->set_sensitive(true);
			_disconnect_button->set_sensitive(false);
			_disconnect_button->set_label("gtk-disconnect");
			_progress_label->set_text(string("Disconnected"));
		}
		return false;
	} else {
		return true;
	}
}

void
ConnectWindow::quit()
{
	_quit_flag = true;
	Gtk::Main::quit();
}

} // namespace GUI
} // namespace Ingen
