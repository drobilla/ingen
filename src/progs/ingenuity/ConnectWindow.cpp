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

#include "ConnectWindow.h"
#include <string>
#include <time.h>
#include <sys/time.h>
#include "interface/ClientKey.h"
#include "interface/ClientInterface.h"
#include "ThreadedSigClientInterface.h"
#include "Controller.h"
#include "OSCListener.h"
#include "Store.h"
#include "PatchController.h"
#include "PatchModel.h"
#include "App.h"

namespace Ingenuity {


ConnectWindow::ConnectWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::Dialog(cobject)
, _client(NULL)
{
	xml->get_widget("connect_icon",                 _icon);
	xml->get_widget("connect_progress_bar",         _progress_bar);
	xml->get_widget("connect_progress_label",       _progress_label);
	xml->get_widget("connect_server_radiobutton",   _server_radio);
	xml->get_widget("connect_url_entry",            _url_entry);
	xml->get_widget("connect_launch_radiobutton",   _launch_radio);
	xml->get_widget("connect_port_spinbutton",      _port_spinbutton);
	xml->get_widget("connect_internal_radiobutton", _internal_radio);
	xml->get_widget("connect_disconnect_button",    _disconnect_button);
	xml->get_widget("connect_connect_button",       _connect_button);
	xml->get_widget("connect_quit_button",          _quit_button);
	
	_server_radio->signal_toggled().connect(sigc::mem_fun(this, &ConnectWindow::server_toggled));
	_launch_radio->signal_toggled().connect(sigc::mem_fun(this, &ConnectWindow::launch_toggled));
	_internal_radio->signal_clicked().connect(sigc::mem_fun(this, &ConnectWindow::internal_toggled));
	_disconnect_button->signal_clicked().connect(sigc::mem_fun(this, &ConnectWindow::disconnect));
	_connect_button->signal_clicked().connect(sigc::mem_fun(this, &ConnectWindow::connect));
	_quit_button->signal_clicked().connect(sigc::mem_fun(this, &ConnectWindow::quit));
}


void
ConnectWindow::start(CountedPtr<Ingen::Shared::ClientInterface> client)
{
	_client = client;
	resize(100, 100);
	show();
}

void
ConnectWindow::init()
{
	_icon->set(Gtk::Stock::DISCONNECT, Gtk::ICON_SIZE_LARGE_TOOLBAR);
	_progress_bar->set_fraction(0.0);
	_url_entry->set_sensitive(true);
	_connect_button->set_sensitive(true);
	_disconnect_button->set_sensitive(false);
	_port_spinbutton->set_sensitive(false);
	_launch_radio->set_sensitive(true);
	_internal_radio->set_sensitive(false);
	server_toggled();
		
	_progress_label->set_text(string("Disconnected"));
}

void
ConnectWindow::connect()
{
	_connect_button->set_sensitive(false);

	if (_server_radio->get_active()) {
		Controller::instance().set_engine_url(_url_entry->get_text());
		Glib::signal_timeout().connect(
			sigc::mem_fun(this, &ConnectWindow::gtk_callback), 100);

	} else if (_launch_radio->get_active()) {
		int port = _port_spinbutton->get_value_as_int();
		char port_str[6];
		snprintf(port_str, 6, "%u", port);
		const string port_arg = string("--port=").append(port_str);
		Controller::instance().set_engine_url(
			string("osc.udp://localhost:").append(port_str));
		
		if (fork() == 0) { // child
			cerr << "Executing 'om " << port_arg << "' ..." << endl; 
			execlp("om", port_arg.c_str(), 0);
		} else {
			Glib::signal_timeout().connect(
				sigc::mem_fun(this, &ConnectWindow::gtk_callback), 100);
		}
	}
}


void
ConnectWindow::disconnect()
{
	_progress_bar->set_fraction(0.0);
	_connect_button->set_sensitive(false);
	_disconnect_button->set_sensitive(false);

	App::instance().disconnect();

	init();
}


void
ConnectWindow::quit()
{
	if (Controller::instance().is_attached()) {
		Gtk::MessageDialog d(*this, "This will exit Ingenuity, but the engine will "
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
}


void
ConnectWindow::launch_toggled()
{
	_url_entry->set_sensitive(false);
	_port_spinbutton->set_sensitive(true);
}


void
ConnectWindow::internal_toggled()
{
	// Not quite yet...
	_url_entry->set_sensitive(false);
	_port_spinbutton->set_sensitive(false);
}


bool
ConnectWindow::gtk_callback()
{
	/* This isn't very nice (isn't threaded), but better than no dialog at 
	 * all like before :)
	 */
	
	// Timing stuff for repeated attach attempts
	timeval now;
	gettimeofday(&now, NULL);
	static timeval last = now;
	
	static int stage = 0;
	
	/* Connecting to engine */
	if (stage == 0) {
		// FIXME
		//assert(!Controller::instance().is_attached());
		_progress_label->set_text(string("Connecting to engine at ").append(
			Controller::instance().engine_url()).append("..."));
		present();
		Controller::instance().attach();
		++stage;
	} else if (stage == 1) {
		if (Controller::instance().is_attached()) {
			Controller::instance().activate();
			++stage;
		} else {
			const float ms_since_last = (now.tv_sec - last.tv_sec) * 1000.0f +
				(now.tv_usec - last.tv_usec) * 0.001f;
			if (ms_since_last > 1000) {
				Controller::instance().attach();
				last = now;
			}
		}
	} else if (stage == 2) {
		_progress_label->set_text(string("Registering as client..."));
		//Controller::instance().register_client(Controller::instance().client_hooks());
		// FIXME
		//auto_ptr<ClientInterface> client(new ThreadedSigClientInterface();
		Controller::instance().register_client(ClientKey(), _client);
		Controller::instance().load_plugins();
		++stage;
	} else if (stage == 3) {
		// Register idle callback to process events and whatnot
		// (Gtk refreshes at priority G_PRIORITY_HIGH_IDLE+20)
		/*Glib::signal_timeout().connect(
			sigc::mem_fun(this, &App::idle_callback), 100, G_PRIORITY_HIGH_IDLE);*/
		//Glib::signal_idle().connect(sigc::mem_fun(this, &App::idle_callback));
	/*	Glib::signal_timeout().connect(
			sigc::mem_fun((ThreadedSigClientInterface*)_client, &ThreadedSigClientInterface::emit_signals),
			5, G_PRIORITY_DEFAULT_IDLE);*/
		
		_progress_label->set_text(string("Requesting plugins..."));
		Controller::instance().request_plugins();
		++stage;
	} else if (stage == 4) {
		// Wait for first plugins message
		if (Store::instance().plugins().size() > 0) {
			_progress_label->set_text(string("Receiving plugins..."));
			++stage;
		}
	} else if (stage == 5) {
		// FIXME
		/*if (Store::instance().plugins().size() < _client->num_plugins()) {
			static char buf[32];
			snprintf(buf, 32, "%zu/%zu", Store::instance().plugins().size(),
				ThreadedSigClientInterface::instance()->num_plugins());
			_progress_bar->set_text(Glib::ustring(buf));
			_progress_bar->set_fraction(
				Store::instance().plugins().size() / (double)_client->num_plugins());
		} else {*/
			_progress_bar->set_text("");
			++stage;
		//}
	} else if (stage == 6) {
		_progress_label->set_text(string("Waiting for root patch..."));
		Controller::instance().request_all_objects();
		++stage;
	} else if (stage == 7) {
		if (Store::instance().num_objects() > 0) {
			CountedPtr<PatchModel> root = Store::instance().object("/");
			assert(root);
			PatchController* root_controller = new PatchController(root);
			root_controller->show_patch_window();
			++stage;
		}
	} else if (stage == 8) {
		_progress_label->set_text(string("Connected to engine at ").append(
			Controller::instance().engine_url()));
		++stage;
	} else if (stage == 9) {
		stage = -1;
		hide();
	}
	
	if (stage != 5) // yeah, ew
		_progress_bar->pulse();
	
	if (stage == -1) { // finished connecting
		_icon->set(Gtk::Stock::CONNECT, Gtk::ICON_SIZE_LARGE_TOOLBAR);
		_progress_bar->set_fraction(1.0);
		_url_entry->set_sensitive(false);
		_connect_button->set_sensitive(false);
		_disconnect_button->set_sensitive(true);
		_port_spinbutton->set_sensitive(false);
		_launch_radio->set_sensitive(false);
		_internal_radio->set_sensitive(false);
		stage = 0; // set ourselves up for next time (if there is one)
		return false; // deregister this callback
	} else {
		return true;
	}
}


} // namespace Ingenuity
