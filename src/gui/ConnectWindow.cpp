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

#include <string>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <raul/Process.hpp>
#include "config.h"
#include "interface/EngineInterface.hpp"
#include "module/World.hpp"
#include "engine/tuning.hpp"
#include "engine/Engine.hpp"
#include "engine/QueuedEngineInterface.hpp"
#ifdef HAVE_SOUP
#include "client/HTTPClientReceiver.hpp"
#endif
#ifdef HAVE_LIBLO
#include "client/OSCClientReceiver.hpp"
#include "client/OSCEngineSender.hpp"
#endif
#include "client/ThreadedSigClientInterface.hpp"
#include "client/ClientStore.hpp"
#include "client/PatchModel.hpp"
#include "module/Module.hpp"
#include "App.hpp"
#include "WindowFactory.hpp"
#include "ConnectWindow.hpp"
using Ingen::QueuedEngineInterface;
using Ingen::Client::ThreadedSigClientInterface;
using namespace std;

namespace Ingen {
namespace GUI {


// ConnectWindow


ConnectWindow::ConnectWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Gtk::Dialog(cobject)
	, _xml(xml)
	, _mode(CONNECT_REMOTE)
	, _ping_id(-1)
	, _attached(false)
	, _widgets_loaded(false)
	, _connect_stage(0)
	, _new_engine(NULL)
{
}


void
ConnectWindow::start(Ingen::Shared::World* world)
{
	if (world->local_engine) {
		_mode = INTERNAL;
		if (_widgets_loaded)
			_internal_radio->set_active(true);
	}
	
	set_connected_to(world->engine);

	connect(true);
}


void
ConnectWindow::set_connected_to(SharedPtr<Shared::EngineInterface> engine)
{
	App::instance().world()->engine = engine;
	
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

		if (_new_engine)
			_internal_radio->set_sensitive(true);
		else
			_internal_radio->set_sensitive(false);

        _server_radio->set_sensitive(true);
        _launch_radio->set_sensitive(true);

        if (_mode == CONNECT_REMOTE )
            _url_entry->set_sensitive(true);
        else if (_mode == LAUNCH_REMOTE )
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
 * This will create the EngineInterface and ClientInterface and initialize
 * the App with them.
 */
void
ConnectWindow::connect(bool existing)
{
	if (_attached)
		_attached = false;

	assert(!App::instance().client());

	_connect_stage = 0;
	set_connecting_widget_states();
		
	Ingen::Shared::World* world = App::instance().world();

#ifdef HAVE_LIBLO
	if (_mode == CONNECT_REMOTE) {
		if (!existing) {
			const string url = (_widgets_loaded ? (string)_url_entry->get_text() : world->engine->uri());
			world->engine = SharedPtr<EngineInterface>(new OSCEngineSender(url));
		}
	
		SharedPtr<ThreadedSigClientInterface> tsci(new ThreadedSigClientInterface(1024));
		SharedPtr<Raul::Deletable> client;
		
		const string& uri = world->engine->uri();
		const string& scheme = uri.substr(0, uri.find(":"));
		if (scheme == "osc.udp" || scheme == "osc.tcp")
			client = SharedPtr<OSCClientReceiver>(new OSCClientReceiver(16181, tsci)); // FIXME: port
#ifdef HAVE_SOUP
		else if (scheme == "http")
			client = SharedPtr<HTTPClientReceiver>(new HTTPClientReceiver(world, uri, tsci));
#endif
		
		App::instance().attach(tsci, client);
		App::instance().register_callbacks();

		Glib::signal_timeout().connect(
			sigc::mem_fun(this, &ConnectWindow::gtk_callback), 40);

	} else if (_mode == LAUNCH_REMOTE) {

		int port = _port_spinbutton->get_value_as_int();
		char port_str[6];
		snprintf(port_str, 6, "%u", port);
		const string cmd = string("ingen -e --engine-port=").append(port_str);

		if (Raul::Process::launch(cmd)) {
			world->engine = SharedPtr<EngineInterface>(
					new OSCEngineSender(string("osc.udp://localhost:").append(port_str)));

			// FIXME: static args
			SharedPtr<ThreadedSigClientInterface> tsci(new ThreadedSigClientInterface(1024));
			SharedPtr<OSCClientReceiver> client(new OSCClientReceiver(16181, tsci));
		
			App::instance().attach(tsci, client);
			App::instance().register_callbacks();

			Glib::signal_timeout().connect(
					sigc::mem_fun(this, &ConnectWindow::gtk_callback), 40);

		} else {
			cerr << "Failed to launch ingen process." << endl;
		}

	} else
#endif
	if (_mode == INTERNAL) {
		if ( ! world->local_engine) {
			assert(_new_engine);
			world->local_engine = SharedPtr<Engine>(_new_engine(world));
		}
		
		if ( ! world->engine)
			world->engine = world->local_engine->new_queued_interface();
		
		SharedPtr<SigClientInterface> client(new SigClientInterface());
		
		world->local_engine->start_jack_driver();
		world->local_engine->activate(1); // FIXME: parallelism
		
		App::instance().attach(client);
		App::instance().register_callbacks();
		
		Glib::signal_timeout().connect(
			sigc::mem_fun(this, &ConnectWindow::gtk_callback), 10);
	}
}


void
ConnectWindow::disconnect()
{
	_connect_stage = -1;
	_attached = false;

	App::instance().detach();
	set_connected_to(SharedPtr<Ingen::Shared::EngineInterface>());

	if (!_widgets_loaded)
		return;

	_progress_bar->set_fraction(0.0);
	_connect_button->set_sensitive(false);
	_disconnect_button->set_sensitive(false);
	
	_connect_button->set_sensitive(true);
	_disconnect_button->set_sensitive(false);
}


void
ConnectWindow::on_show()
{
	if (!_widgets_loaded) {
		load_widgets();
		if (_attached)
			set_connected_to(App::instance().engine());
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
	_xml->get_widget("connect_disconnect_button",    _disconnect_button);
	_xml->get_widget("connect_connect_button",       _connect_button);
	_xml->get_widget("connect_quit_button",          _quit_button);

	_server_radio->signal_toggled().connect(sigc::mem_fun(this, &ConnectWindow::server_toggled));
	_launch_radio->signal_toggled().connect(sigc::mem_fun(this, &ConnectWindow::launch_toggled));
	_internal_radio->signal_clicked().connect(sigc::mem_fun(this, &ConnectWindow::internal_toggled));
	_disconnect_button->signal_clicked().connect(sigc::mem_fun(this, &ConnectWindow::disconnect));
	_connect_button->signal_clicked().connect(sigc::bind(
			sigc::mem_fun(this, &ConnectWindow::connect), false));
	_quit_button->signal_clicked().connect(sigc::mem_fun(this, &ConnectWindow::quit));
	
	_progress_bar->set_pulse_step(0.01);
	_widgets_loaded = true;
	
	_engine_module = Ingen::Shared::load_module("ingen_engine");
	if (!_engine_module)
		cerr << "Unable to load ingen_engine module, internal engine unavailable." << endl;
	bool found = _engine_module->get_symbol("new_engine", (void*&)_new_engine);
	if (!found) {
		cerr << "Unable to find module entry point, internal engine unavailable." << endl;
		_engine_module.reset();
	}

    server_toggled();
}


void
ConnectWindow::on_hide()
{
	Gtk::Dialog::on_hide();
	if (!_attached)
		Gtk::Main::quit();
}


void
ConnectWindow::quit()
{
	if (_attached) {
		Gtk::MessageDialog d(*this, "This will exit the GUI, but the engine will "
			"remain running (if it is remote).\n\nAre you sure you want to quit?",
			true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE, true);
			d.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
			d.add_button(Gtk::Stock::QUIT, Gtk::RESPONSE_CLOSE);
		int ret = d.run();
		if (ret == Gtk::RESPONSE_CLOSE)
			Gtk::Main::quit();
	} else {
		Gtk::Main::quit();
	}
}


void
ConnectWindow::server_toggled()
{
	_url_entry->set_sensitive(true);
	_port_spinbutton->set_sensitive(false);
	_mode = CONNECT_REMOTE;
}


void
ConnectWindow::launch_toggled()
{
	_url_entry->set_sensitive(false);
	_port_spinbutton->set_sensitive(true);
	_mode = LAUNCH_REMOTE;
}


void
ConnectWindow::internal_toggled()
{
	_url_entry->set_sensitive(false);
	_port_spinbutton->set_sensitive(false);
	_mode = INTERNAL;
}


bool
ConnectWindow::gtk_callback()
{
	/* If I call this a "state machine" it's not ugly code any more */
	
	// Timing stuff for repeated attach attempts
	timeval now;
	gettimeofday(&now, NULL);
	static const timeval start = now;
	static timeval last = now;

	// Show if attempted connection goes on for a noticeable amount of time
	if (!is_visible()) {
		const float ms_since_start = (now.tv_sec - start.tv_sec) * 1000.0f +
				(now.tv_usec - start.tv_usec) * 0.001f;
		if (ms_since_start > 500) {
			present();
			set_connecting_widget_states();
		}
	}
	
	/* Connecting to engine */
	if (_connect_stage == 0) {

		_attached = false;

		assert(App::instance().engine());
		assert(App::instance().client());

		App::instance().client()->signal_response_ok.connect(
				sigc::mem_fun(this, &ConnectWindow::on_response));
		
		_ping_id = abs(rand()) / 2 * 2; // avoid -1
		App::instance().engine()->set_next_response_id(_ping_id);
		App::instance().engine()->ping();
		
		if (_widgets_loaded) {
			_progress_label->set_text("Connecting to engine...");
			_progress_bar->set_pulse_step(0.01);
		}

		++_connect_stage;

	} else if (_connect_stage == 1) {
		if (_attached || App::instance().client()->enabled()) {
			App::instance().engine()->activate();
			++_connect_stage;
		} else {
			const float ms_since_last = (now.tv_sec - last.tv_sec) * 1000.0f +
					(now.tv_usec - last.tv_usec) * 0.001f;
			if (ms_since_last > 1000) {
				App::instance().engine()->set_next_response_id(_ping_id);
				App::instance().engine()->ping();
				last = now;
			}
		}
	} else if (_connect_stage == 2) {
		App::instance().engine()->request_all_objects();
		if (_widgets_loaded)
			_progress_label->set_text(string("Requesting root patch..."));
		++_connect_stage;
	} else if (_connect_stage == 3) {
		if (App::instance().store()->size() > 0) {
			SharedPtr<PatchModel> root = PtrCast<PatchModel>(App::instance().store()->object("/"));
			if (root) {
				set_connected_to(App::instance().engine());
				App::instance().window_factory()->present_patch(root);
				App::instance().engine()->load_plugins();
				if (_widgets_loaded)
					_progress_label->set_text(string("Loading plugins..."));
				++_connect_stage;
			}
		}
	} else if (_connect_stage == 4) {
		App::instance().engine()->request_plugins();
		hide();
		if (_widgets_loaded)
			_progress_label->set_text("Connected to engine");
		_connect_stage = 0; // set ourselves up for next time (if there is one)
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


} // namespace GUI
} // namespace Ingen
