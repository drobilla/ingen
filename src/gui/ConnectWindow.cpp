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

#include <stdlib.h>

#include <limits>
#include <sstream>
#include <string>

#include <gtkmm/stock.h>

#include "raul/Process.hpp"

#include "ingen/Configuration.hpp"
#include "ingen/EngineBase.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/Module.hpp"
#include "ingen/World.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/GraphModel.hpp"
#include "ingen/client/SocketClient.hpp"
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
	, _connect_uri("unix:///tmp/ingen.sock")
	, _ping_id(-1)
	, _attached(false)
	, _finished_connecting(false)
	, _widgets_loaded(false)
	, _connect_stage(0)
	, _quit_flag(false)
{
}

void
ConnectWindow::error(const std::string& msg)
{
	if (!is_visible()) {
		present();
		set_connecting_widget_states();
	}

	if (_progress_label) {
		_progress_label->set_text(msg);
	}

	if (_app && _app->world()) {
		_app->world()->log().error(msg + "\n");
	}
}

void
ConnectWindow::start(App& app, Ingen::World* world)
{
	_app = &app;

	if (world->engine()) {
		_mode = Mode::INTERNAL;
	}

	set_connected_to(world->interface());
	connect(bool(world->interface()));
}

void
ConnectWindow::ingen_response(int32_t            id,
                              Status             status,
                              const std::string& subject)
{
	if (id == _ping_id) {
		if (status != Status::SUCCESS) {
			error("Failed to get root patch");
		} else {
			_attached = true;
		}
	}
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
		_url_entry->set_text(engine->uri());
		_connect_button->set_sensitive(false);
		_disconnect_button->set_label("gtk-disconnect");
		_disconnect_button->set_sensitive(true);
		_port_spinbutton->set_sensitive(false);
		_launch_radio->set_sensitive(false);
		_internal_radio->set_sensitive(false);
		_activate_button->set_sensitive(true);
		_deactivate_button->set_sensitive(true);

	} else {
		_icon->set(Gtk::Stock::DISCONNECT, Gtk::ICON_SIZE_LARGE_TOOLBAR);
		_progress_bar->set_fraction(0.0);
		_connect_button->set_sensitive(true);
		_disconnect_button->set_sensitive(false);
		_internal_radio->set_sensitive(true);
		_server_radio->set_sensitive(true);
		_launch_radio->set_sensitive(true);
		_activate_button->set_sensitive(false);
		_deactivate_button->set_sensitive(false);

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

bool
ConnectWindow::connect_remote(const Raul::URI& uri)
{
	Ingen::World* world = _app->world();

	SPtr<ThreadedSigClientInterface> tsci(
		new Client::ThreadedSigClientInterface(1024));

	SPtr<Ingen::Interface> iface(world->new_interface(uri, tsci));
	if (iface) {
		world->set_interface(iface);
		_app->attach(tsci);
		_app->register_callbacks();
		return true;
	}

	return false;
}

/** Set up initial connect stage and launch connect callback. */
void
ConnectWindow::connect(bool existing)
{
	if (_app->client()) {
		error("Already connected");
		return;
	} else if (_attached) {
		_attached = false;
	}

	set_connecting_widget_states();
	_connect_stage = 0;

	Ingen::World* world = _app->world();

	if (_mode == Mode::CONNECT_REMOTE) {
		std::string uri_str = world->conf().option("connect").ptr<char>();
		if (existing) {
			uri_str = world->interface()->uri();
			_connect_stage = 1;
			SPtr<Client::SocketClient> client = dynamic_ptr_cast<Client::SocketClient>(world->interface());
			if (client) {
				_app->attach(dynamic_ptr_cast<Client::SigClientInterface>(client->respondee()));
				_app->register_callbacks();
			} else {
				error("Connected with invalid client interface type");
				return;
			}
		} else if (_widgets_loaded) {
			uri_str = _url_entry->get_text();
		}

		if (!Raul::URI::is_valid(uri_str)) {
			error((fmt("Invalid socket URI %1%") % uri_str).str());
			return;
		}

		_connect_uri = Raul::URI(uri_str);

	} else if (_mode == Mode::LAUNCH_REMOTE) {
		const std::string port  = std::to_string(_port_spinbutton->get_value_as_int());
		const char*       cmd[] = { "ingen", "-e", "-E", port.c_str(), NULL };

		if (!Raul::Process::launch(cmd)) {
			error("Failed to launch engine process");
			return;
		}

		_connect_uri = Raul::URI(std::string("tcp://localhost:") + port);

	} else if (_mode == Mode::INTERNAL) {
		if (!world->engine()) {
			if (!world->load_module("server")) {
				error("Failed to load server module");
				return;
			} else if (!world->load_module("jack")) {
				error("Failed to load jack module");
				return;
			} else if (!world->engine()->activate()) {
				error("Failed to activate engine");
				return;
			}
		}
	}

	set_connecting_widget_states();
	if (_widgets_loaded) {
		_progress_label->set_text("Connecting...");
	}
	Glib::signal_timeout().connect(
		sigc::mem_fun(this, &ConnectWindow::gtk_callback), 33);
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
	if (!_app->interface()) {
		return;
	}

	_app->interface()->set_property(Raul::URI("ingen:/driver"),
	                                _app->uris().ingen_enabled,
	                                _app->forge().make(true));
}

void
ConnectWindow::deactivate()
{
	if (!_app->interface()) {
		return;
	}

	_app->interface()->set_property(Raul::URI("ingen:/driver"),
	                                _app->uris().ingen_enabled,
	                                _app->forge().make(false));
}

void
ConnectWindow::on_show()
{
	if (!_widgets_loaded) {
		load_widgets();
	}

	if (_attached) {
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

	_url_entry->set_text(_app->world()->conf().option("connect").ptr<char>());
	if (Raul::URI::is_valid(_url_entry->get_text())) {
		_connect_uri = Raul::URI(_url_entry->get_text());
	}

	_port_spinbutton->set_range(1, std::numeric_limits<uint16_t>::max());
	_port_spinbutton->set_increments(1, 100);
	_port_spinbutton->set_value(
		_app->world()->conf().option("engine-port").get<int32_t>());

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

void
ConnectWindow::next_stage()
{
	static const char* labels[] = {
		"Connecting...",
		"Pinging engine...",
		"Attaching to engine...",
		"Requesting root graph...",
		"Loading plugins...",
		"Connected"
	};


	++_connect_stage;
	if (_widgets_loaded) {
		_progress_label->set_text(labels[_connect_stage]);
	}
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
	static const timeval start    = now;
	static timeval       last     = now;
	static unsigned      attempts = 0;

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
		const float ms_since_last = (now.tv_sec - last.tv_sec) * 1000.0f +
			(now.tv_usec - last.tv_usec) * 0.001f;
		if (ms_since_last >= 250) {
			last = now;
			if (_mode == Mode::INTERNAL) {
				SPtr<SigClientInterface> client(new SigClientInterface());
				_app->world()->interface()->set_respondee(client);
				_app->attach(client);
				_app->register_callbacks();
				next_stage();
			} else if (connect_remote(_connect_uri)) {
				next_stage();
			}
		}
	} else if (_connect_stage == 1) {
		_attached = false;
		_app->client()->signal_response().connect(
			sigc::mem_fun(this, &ConnectWindow::ingen_response));

		_ping_id = g_random_int_range(1, std::numeric_limits<int32_t>::max());
		_app->interface()->set_response_id(_ping_id);
		_app->interface()->get(Raul::URI("ingen:/engine"));
		last     = now;
		attempts = 0;
		next_stage();

	} else if (_connect_stage == 2) {
		if (_attached) {
			next_stage();
		} else {
			const float ms_since_last = (now.tv_sec - last.tv_sec) * 1000.0f +
				(now.tv_usec - last.tv_usec) * 0.001f;
			if (attempts > 10) {
				error("Failed to ping engine");
				_connect_stage = -1;
			} else if (ms_since_last > 1000) {
				_app->interface()->set_response_id(_ping_id);
				_app->interface()->get(Raul::URI("ingen:/engine"));
				last = now;
				++attempts;
			}
		}
	} else if (_connect_stage == 3) {
		_app->interface()->get(Raul::URI(Node::main_uri() + "/"));
		next_stage();
	} else if (_connect_stage == 4) {
		if (_app->store()->size() > 0) {
			SPtr<const GraphModel> root = dynamic_ptr_cast<const GraphModel>(
				_app->store()->object(Raul::Path("/")));
			if (root) {
				set_connected_to(_app->interface());
				_app->window_factory()->present_graph(root);
				next_stage();
			}
		}
	} else if (_connect_stage == 5) {
		hide();
		_connect_stage = 0; // set ourselves up for next time (if there is one)
		_finished_connecting = true;
		_app->interface()->set_response_id(1);
		return false; // deregister this callback
	}

	if (_widgets_loaded) {
		_progress_bar->pulse();
	}

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
