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

namespace OmGtk {


ConnectWindow::ConnectWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::Dialog(cobject)
, _client(NULL)
{
	xml->get_widget("connect_progress_bar",  _progress_bar);
	xml->get_widget("connect_label",         _label);
	xml->get_widget("connect_launch_button", _launch_button);
	xml->get_widget("connect_cancel_button", _cancel_button);
	
	assert(_progress_bar);
	assert(_label);
	assert(_launch_button);
	assert(_cancel_button);

	_launch_button->signal_clicked().connect(sigc::mem_fun(this, &ConnectWindow::launch_engine));
	_cancel_button->signal_clicked().connect(sigc::ptr_fun(&Gtk::Main::quit));
}


void
ConnectWindow::start(CountedPtr<Om::Shared::ClientInterface> client)
{
	_client = client;

	Glib::signal_timeout().connect(
		sigc::mem_fun(this, &ConnectWindow::gtk_callback), 100);
	
	resize(100, 100);
}


void
ConnectWindow::launch_engine()
{
	if (fork() == 0) {
		//cerr << "Launching engine..";
		execlp("om", NULL);
	}
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
		_label->set_text(string("Connecting to engine at ").append(
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
		_label->set_text(string("Registering as client..."));
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
		
		_label->set_text(string("Requesting plugins..."));
		Controller::instance().request_plugins();
		++stage;
	} else if (stage == 4) {
		// Wait for first plugins message
		if (Store::instance().plugins().size() > 0) {
			_label->set_text(string("Receiving plugins..."));
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
		_label->set_text(string("Waiting for root patch..."));
		Controller::instance().request_all_objects();
		++stage;
	} else if (stage == 7) {
		if (Store::instance().num_objects() > 0)
			++stage;
	} else if (stage == 8) {
		stage = -1;
		hide(); // FIXME: actually destroy window to save mem?
	}
	
	if (stage != 5) // yeah, ew
		_progress_bar->pulse();
	
	if (stage == -1) { // finished connecting
		return false; // deregister this callback
	} else {
		return true;
	}
}


} // namespace OmGtk
