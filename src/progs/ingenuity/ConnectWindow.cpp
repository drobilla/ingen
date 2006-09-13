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

#include <string>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include "config.h"
#include "ConnectWindow.h"
#include "interface/ClientKey.h"
#include "OSCModelEngineInterface.h"
#include "OSCClientReceiver.h"
#include "ThreadedSigClientInterface.h"
#include "Store.h"
#include "PatchModel.h"
#include "App.h"
#include "WindowFactory.h"
#ifdef MONOLITHIC_INGENUITY
	#include "engine/QueuedEngineInterface.h"
	#include "engine/Engine.h"
	#include "engine/DirectResponder.h"
	#include "engine/tuning.h"
#endif
using Ingen::Client::ThreadedSigClientInterface;
using Ingen::QueuedEngineInterface;

namespace Ingenuity {


// Paste together some interfaces to get the combination we want


struct OSCSigEmitter : public OSCClientReceiver, public ThreadedSigClientInterface {
public:
	OSCSigEmitter(size_t queue_size, int listen_port)
	: Ingen::Shared::ClientInterface()
	, OSCClientReceiver(listen_port)
	, ThreadedSigClientInterface(queue_size)
	{
	}
};


struct QueuedModelEngineInterface : public QueuedEngineInterface, public ModelEngineInterface {
	QueuedModelEngineInterface(CountedPtr<Ingen::Engine> engine)
	: Ingen::Shared::EngineInterface()
	, Ingen::QueuedEngineInterface(engine, Ingen::event_queue_size, Ingen::event_queue_size)
	{
		QueuedEventSource::start();
	}
};


// ConnectWindow


ConnectWindow::ConnectWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::Dialog(cobject)
, _mode(CONNECT_REMOTE)
, _ping_id(-1)
, _attached(false)
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
ConnectWindow::start()
{
	resize(100, 100);
	init();
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
#ifdef MONOLITHIC_INGENUITY
	_internal_radio->set_sensitive(true);
#else
	_internal_radio->set_sensitive(false);
#endif
	server_toggled();
		
	_progress_label->set_text(string("Disconnected"));
}


/** Launch (if applicable) and connect to the Engine.
 *
 * This will create the EngineInterface and ClientInterface and initialize
 * the App with them.
 */
void
ConnectWindow::connect()
{
	assert(!_attached);
	assert(!App::instance().engine());
	assert(!App::instance().client());

	_connect_button->set_sensitive(false);

	if (_mode == CONNECT_REMOTE) {
		CountedPtr<ModelEngineInterface> engine(new OSCModelEngineInterface(_url_entry->get_text()));
		OSCSigEmitter* ose = new OSCSigEmitter(1024, 16181); // FIXME: args
		CountedPtr<SigClientInterface> client(ose);
		App::instance().attach(engine, client);

		Glib::signal_timeout().connect(
			sigc::mem_fun(this, &ConnectWindow::gtk_callback), 100);
		
		Glib::signal_timeout().connect(
			sigc::mem_fun(ose, &ThreadedSigClientInterface::emit_signals), 2, G_PRIORITY_HIGH_IDLE);

	} else if (_mode == LAUNCH_REMOTE) {
		/*
		int port = _port_spinbutton->get_value_as_int();
		char port_str[6];
		snprintf(port_str, 6, "%u", port);
		const string port_arg = string("--port=").append(port_str);
		App::instance().engine()->set_engine_url(
			string("osc.udp://localhost:").append(port_str));
		
		if (fork() == 0) { // child
			cerr << "Executing 'ingen " << port_arg << "' ..." << endl; 
			execlp("ingen", port_arg.c_str(), (char*)NULL);
		} else {
			Glib::signal_timeout().connect(
				sigc::mem_fun(this, &ConnectWindow::gtk_callback), 100);
		}*/
		throw;
#ifdef MONOLITHIC_INGENUITY
	} else if (_mode == INTERNAL) {
		CountedPtr<Ingen::Engine> engine(new Ingen::Engine());
		QueuedModelEngineInterface* qmei = new QueuedModelEngineInterface(engine);
		CountedPtr<ModelEngineInterface> engine_interface(qmei);
		ThreadedSigClientInterface* tsci = new ThreadedSigClientInterface(Ingen::event_queue_size);
		CountedPtr<SigClientInterface> client(tsci);
		
		App::instance().attach(engine_interface, client);

		qmei->set_responder(CountedPtr<Ingen::Responder>(new Ingen::DirectResponder(client, 1)));
		engine->set_event_source(qmei);
		
		Glib::signal_timeout().connect(
			sigc::mem_fun(engine.get(), &Ingen::Engine::main_iteration), 1000);
		
		Glib::signal_timeout().connect(
			sigc::mem_fun(this, &ConnectWindow::gtk_callback), 100);
		
		Glib::signal_timeout().connect(
			sigc::mem_fun(tsci, &ThreadedSigClientInterface::emit_signals), 2, G_PRIORITY_HIGH_IDLE);
#endif
	}
}


void
ConnectWindow::disconnect()
{
	_attached = false;

	_progress_bar->set_fraction(0.0);
	_connect_button->set_sensitive(false);
	_disconnect_button->set_sensitive(false);

	App::instance().disconnect();

	init();
}


void
ConnectWindow::quit()
{
	if (_attached) {
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

		assert(!_attached);
		assert(App::instance().engine());
		assert(App::instance().client());

		// FIXME
		//assert(!App::instance().engine()->is_attached());
		_progress_label->set_text("Connecting to engine...");
		present();

		App::instance().client()->response_sig.connect(sigc::mem_fun(this, &ConnectWindow::response_received));

		_ping_id = rand();
		while (_ping_id == -1)
			_ping_id = rand();

		App::instance().engine()->set_next_response_id(_ping_id);
		App::instance().engine()->ping();
		++stage;


	} else if (stage == 1) {
		if (_attached) {
			App::instance().engine()->activate();
			++stage;
		} else {
			const float ms_since_last = (now.tv_sec - last.tv_sec) * 1000.0f +
				(now.tv_usec - last.tv_usec) * 0.001f;
			if (ms_since_last > 1000) {
				App::instance().engine()->set_next_response_id(_ping_id);
				App::instance().engine()->ping();
				last = now;
			}
		}
	} else if (stage == 2) {
		_progress_label->set_text(string("Registering as client..."));
		//App::instance().engine()->register_client(App::instance().engine()->client_hooks());
		// FIXME
		//auto_ptr<ClientInterface> client(new ThreadedSigClientInterface();
		App::instance().engine()->register_client(ClientKey(), App::instance().client());
		App::instance().engine()->load_plugins();
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
		App::instance().engine()->request_plugins();
		++stage;
	} else if (stage == 4) {
		// Wait for first plugins message
		if (App::instance().store()->plugins().size() > 0) {
			_progress_label->set_text(string("Receiving plugins..."));
			++stage;
		}
	} else if (stage == 5) {
		// FIXME
		/*if (App::instance().store().plugins().size() < _client->num_plugins()) {
			static char buf[32];
			snprintf(buf, 32, "%zu/%zu", App::instance().store().plugins().size(),
				ThreadedSigClientInterface::instance()->num_plugins());
			_progress_bar->set_text(Glib::ustring(buf));
			_progress_bar->set_fraction(
				App::instance().store().plugins().size() / (double)_client->num_plugins());
		} else {*/
			_progress_bar->set_text("");
			++stage;
		//}
	} else if (stage == 6) {
		_progress_label->set_text(string("Waiting for root patch..."));
		App::instance().engine()->request_all_objects();
		++stage;
	} else if (stage == 7) {
		if (App::instance().store()->num_objects() > 0) {
			CountedPtr<PatchModel> root = PtrCast<PatchModel>(App::instance().store()->object("/"));
			assert(root);
			App::instance().window_factory()->present_patch(root);
			++stage;
		}
	} else if (stage == 8) {
		_progress_label->set_text("Connected to engine");
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
