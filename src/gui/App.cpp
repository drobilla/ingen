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

#include <cassert>
#include <fstream>
#include <string>
#include <utility>

#include <gtk/gtkwindow.h>
#include <gtkmm/stock.h>

#include "ganv/Edge.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/EngineBase.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/World.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/GraphModel.hpp"
#include "ingen/client/ObjectModel.hpp"
#include "ingen/client/SigClientInterface.hpp"
#include "ingen/runtime_paths.hpp"
#include "lilv/lilv.h"
#include "raul/Path.hpp"

#include "App.hpp"
#include "ConnectWindow.hpp"
#include "GraphTreeWindow.hpp"
#include "GraphWindow.hpp"
#include "LoadPluginWindow.hpp"
#include "MessagesWindow.hpp"
#include "NodeModule.hpp"
#include "Port.hpp"
#include "Style.hpp"
#include "SubgraphModule.hpp"
#include "ThreadedLoader.hpp"
#include "WidgetFactory.hpp"
#include "WindowFactory.hpp"

using namespace std;

namespace Raul { class Deletable; }

namespace Ingen {

namespace Client { class PluginModel; }

using namespace Client;

namespace GUI {

class Port;

Gtk::Main* App::_main = 0;

App::App(Ingen::World* world)
	: _style(new Style(*this))
	, _about_dialog(NULL)
	, _window_factory(new WindowFactory(*this))
	, _world(world)
	, _sample_rate(48000)
	, _enable_signal(true)
	, _requested_plugins(false)
{
	_world->conf().load_default("ingen", "gui.ttl");

	WidgetFactory::get_widget_derived("connect_win", _connect_window);
	WidgetFactory::get_widget_derived("messages_win", _messages_window);
	WidgetFactory::get_widget_derived("graph_tree_win", _graph_tree_window);
	WidgetFactory::get_widget("about_win", _about_dialog);
	_connect_window->init_dialog(*this);
	_messages_window->init_window(*this);
	_graph_tree_window->init_window(*this);
	_about_dialog->property_program_name() = "Ingen";
	_about_dialog->property_logo_icon_name() = "ingen";

	PluginModel::set_rdf_world(*world->rdf_world());
	PluginModel::set_lilv_world(world->lilv_world());
}

App::~App()
{
	delete _style;
	delete _window_factory;
}

SPtr<App>
App::create(Ingen::World* world)
{
	// Add RC file for embedded GUI Gtk style
	const std::string rc_path = Ingen::data_file_path("ingen_style.rc");
	Gtk::RC::add_default_file(rc_path);

	_main = Gtk::Main::instance();
	if (!_main) {
		Glib::set_application_name("Ingen");
		gtk_window_set_default_icon_name("ingen");
		_main = new Gtk::Main(&world->argc(), &world->argv());
	}

	App* app = new App(world);

	// Load configuration settings
	app->style()->load_settings();
	app->style()->apply_settings();

	// Set default window icon
	app->_about_dialog->property_program_name() = "Ingen";
	app->_about_dialog->property_logo_icon_name() = "ingen";
	gtk_window_set_default_icon_name("ingen");

	return SPtr<App>(app);
}

void
App::run()
{
	_connect_window->start(*this, world());

	// Run main iterations here until we're attached to the engine. Otherwise
	// with 'ingen -egl' we'd get a bunch of notifications about load
	// immediately before even knowing about the root graph or plugins)
	while (!_connect_window->attached())
		if (_main->iteration())
			break;

	_main->run();
}

void
App::attach(SPtr<SigClientInterface> client)
{
	assert(!_client);
	assert(!_store);
	assert(!_loader);

	if (_world->engine()) {
		_world->engine()->register_client(client);
	}

	_client = client;
	_store  = SPtr<ClientStore>(new ClientStore(_world->uris(), _world->log(), client));
	_loader = SPtr<ThreadedLoader>(new ThreadedLoader(*this, _world->interface()));
	if (!_world->store()) {
		_world->set_store(_store);
	}

	_graph_tree_window->init(*this, *_store);

	_client->signal_response().connect(
		sigc::mem_fun(this, &App::response));
	_client->signal_error().connect(
		sigc::mem_fun(this, &App::error_message));
	_client->signal_property_change().connect(
		sigc::mem_fun(this, &App::property_change));
}

void
App::detach()
{
	if (_world->interface()) {
		_window_factory->clear();
		_store->clear();

		_loader.reset();
		_store.reset();
		_client.reset();
		_world->set_interface(SPtr<Interface>());
	}
}

void
App::request_plugins_if_necessary()
{
	if (!_requested_plugins) {
		_world->interface()->get(Raul::URI("ingen:/plugins"));
		_requested_plugins = true;
	}
}

SPtr<Serialiser>
App::serialiser()
{
	return _world->serialiser();
}

void
App::response(int32_t id, Status status, const std::string& subject)
{
	if (status != Status::SUCCESS) {
		std::string msg = ingen_status_string(status);
		if (!subject.empty()) {
			msg += ": " + subject;
		}
		error_message(msg);
	}
}

void
App::error_message(const string& str)
{
	_messages_window->post(str);

	if (!_messages_window->is_visible())
		_messages_window->present();

	_messages_window->set_urgency_hint(true);
}

void
App::set_property(const Raul::URI& subject,
                  const Raul::URI& key,
                  const Atom&      value)
{
	// Send message to server
	interface()->set_property(subject, key, value);

	/* The server does not feed back set messages (kludge to prevent control
	   feedback and bandwidth wastage, see Delta.cpp).  So, assume everything
	   went as planned here and fire the signal ourselves as if the server
	   feedback came back immediately. */
	_client->signal_property_change().emit(subject, key, value);
}

void
App::property_change(const Raul::URI& subject,
                     const Raul::URI& key,
                     const Atom&      value)
{
	if (subject == Raul::URI("ingen:/engine") && key == uris().param_sampleRate) {
		if (value.type() == forge().Int) {
			log().info(fmt("Sample rate: %1%\n") % uris().forge.str(value));
			_sample_rate = value.get<int32_t>();
		} else {
			log().error("Engine sample rate property is not an integer\n");
		}
	}
}

void
App::port_activity(Port* port)
{
	std::pair<ActivityPorts::iterator, bool> inserted = _activity_ports.insert(make_pair(port, false));
	if (inserted.second)
		inserted.first->second = false;

	port->set_highlighted(true);
}

void
App::activity_port_destroyed(Port* port)
{
	ActivityPorts::iterator i = _activity_ports.find(port);
	if (i != _activity_ports.end())
		_activity_ports.erase(i);

	return;
}

bool
App::animate()
{
	for (ActivityPorts::iterator i = _activity_ports.begin(); i != _activity_ports.end(); ) {
		ActivityPorts::iterator next = i;
		++next;

		if ((*i).second) { // saw it last time, unhighlight and pop
			(*i).first->set_highlighted(false);
			_activity_ports.erase(i);
		} else {
			(*i).second = true;
		}

		i = next;
	}

	return true;
}

/******** Event Handlers ************/

void
App::register_callbacks()
{
	Glib::signal_timeout().connect(
		sigc::mem_fun(*this, &App::gtk_main_iteration), 25, G_PRIORITY_DEFAULT);

	Glib::signal_timeout().connect(
		sigc::mem_fun(*this, &App::animate), 50, G_PRIORITY_DEFAULT);
}

bool
App::gtk_main_iteration()
{
	if (!_client)
		return false;

	if (_world->engine()) {
		if (!_world->engine()->main_iteration()) {
			Gtk::Main::quit();
			return false;
		}
	} else {
		_enable_signal = false;
		_client->emit_signals();
		_enable_signal = true;
	}

	return true;
}

void
App::show_about()
{
	_about_dialog->run();
	_about_dialog->hide();
}

/** Prompt (if necessary) and quit application (if confirmed).
 * @return true iff the application quit.
 */
bool
App::quit(Gtk::Window* dialog_parent)
{
	bool quit = true;
	if (_world->engine() && _connect_window->attached()) {
		Gtk::MessageDialog d(
			"The engine is running in this process.  Quitting will terminate Ingen."
			"\n\n" "Are you sure you want to quit?",
			true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE, true);
		if (dialog_parent) {
			d.set_transient_for(*dialog_parent);
		}
		d.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		d.add_button(Gtk::Stock::QUIT, Gtk::RESPONSE_CLOSE);
		quit = (d.run() == Gtk::RESPONSE_CLOSE);
	}

	if (!quit) {
		return false;
	}

	Gtk::Main::quit();

	try {
		const std::string path = _world->conf().save(
			_world->uri_map(), "ingen", "gui.ttl", Configuration::GUI);
		cout << (fmt("Saved GUI settings to %1%\n") % path);
	} catch (const std::exception& e) {
		cerr << (fmt("Error saving GUI settings (%1%)\n")
		         % e.what());
	}

	return true;
}

bool
App::can_control(const Client::PortModel* port) const
{
	return port->is_a(uris().lv2_ControlPort)
		|| port->is_a(uris().lv2_CVPort)
		|| (port->is_a(uris().atom_AtomPort)
		    && (port->supports(uris().atom_Float)
		        || port->supports(uris().atom_String)));
}

uint32_t
App::sample_rate() const
{
	return _sample_rate;
}

} // namespace GUI
} // namespace Ingen
