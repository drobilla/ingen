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
#include "ingen/EngineBase.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/World.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/ObjectModel.hpp"
#include "ingen/client/GraphModel.hpp"
#include "ingen/client/SigClientInterface.hpp"
#include "ingen/runtime_paths.hpp"
#include "lilv/lilv.h"
#include "raul/Path.hpp"

#include "App.hpp"
#include "Configuration.hpp"
#include "ConnectWindow.hpp"
#include "LoadPluginWindow.hpp"
#include "MessagesWindow.hpp"
#include "NodeModule.hpp"
#include "GraphTreeWindow.hpp"
#include "GraphWindow.hpp"
#include "Port.hpp"
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
	: _configuration(new Configuration(*this))
	, _about_dialog(NULL)
	, _window_factory(new WindowFactory(*this))
	, _world(world)
	, _sample_rate(48000)
	, _enable_signal(true)
{
	Glib::set_application_name("Ingen");
	gtk_window_set_default_icon_name("ingen");

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
	delete _configuration;
	delete _window_factory;
}

SharedPtr<App>
App::create(Ingen::World* world)
{
	_main = new Gtk::Main(&world->argc(), &world->argv());

	App* app = new App(world);

	// Load configuration settings
	app->configuration()->load_settings();
	app->configuration()->apply_settings();

	// Set default window icon
	app->_about_dialog->property_program_name() = "Ingen";
	app->_about_dialog->property_logo_icon_name() = "ingen";
	gtk_window_set_default_icon_name("ingen");

	// Set style for embedded block GUIs
	const string rc_style =
		"style \"ingen_embedded_node_gui_style\" {\n"
		"bg[NORMAL]      = \"#212222\"\n"
		"bg[ACTIVE]      = \"#505050\"\n"
		"bg[PRELIGHT]    = \"#525454\"\n"
		"bg[SELECTED]    = \"#99A0A0\"\n"
		"bg[INSENSITIVE] = \"#F03030\"\n"
		"fg[NORMAL]      = \"#FFFFFF\"\n"
		"fg[ACTIVE]      = \"#FFFFFF\"\n"
		"fg[PRELIGHT]    = \"#FFFFFF\"\n"
		"fg[SELECTED]    = \"#FFFFFF\"\n"
		"fg[INSENSITIVE] = \"#FFFFFF\"\n"
		"}\n"
		"widget \"*ingen_embedded_node_gui_container*\" style \"ingen_embedded_node_gui_style\"\n";

	Gtk::RC::parse_string(rc_style);

	return SharedPtr<App>(app);
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
App::attach(SharedPtr<SigClientInterface> client)
{
	assert(!_client);
	assert(!_store);
	assert(!_loader);

	if (_world->engine()) {
		_world->engine()->register_client(client->uri(), client);
	}

	_client = client;
	_store  = SharedPtr<ClientStore>(new ClientStore(_world->uris(), _world->log(), _world->interface(), client));
	_loader = SharedPtr<ThreadedLoader>(new ThreadedLoader(*this, _world->interface()));

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
		_world->set_interface(SharedPtr<Interface>());
	}
}

SharedPtr<Serialisation::Serialiser>
App::serialiser()
{
	if (!_world->serialiser())
		_world->load_module("serialisation");

	return _world->serialiser();
}

void
App::response(int32_t id, Status status, const std::string& subject)
{
	if (status) {
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
App::property_change(const Raul::URI&  subject,
                     const Raul::URI&  key,
                     const Raul::Atom& value)
{
	if (subject == uris().ingen_engine && key == uris().ingen_sampleRate) {
		if (value.type() == forge().Int) {
			log().info(Raul::fmt("Sample rate: %1%\n") % uris().forge.str(value));
			_sample_rate = value.get_int32();
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
	if (_world->engine()) {
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

	if (quit)
		Gtk::Main::quit();

	return quit;
}

struct IconDestroyNotification {
	IconDestroyNotification(App& a, pair<string, int> k)
		: app(a)
		, key(k)
	{}

	App&              app;
	pair<string, int> key;
};

Glib::RefPtr<Gdk::Pixbuf>
App::icon_from_path(const string& path, int size)
{
	/* If weak references to Glib::Objects are needed somewhere else it will
	   probably be a good idea to create a proper WeakPtr class instead of
	   using raw pointers, but for a single use this will do. */

	Glib::RefPtr<Gdk::Pixbuf> buf;
	if (path.length() == 0)
		return buf;

	Icons::iterator iter = _icons.find(make_pair(path, size));

	if (iter != _icons.end()) {
		// we need to reference manually since the RefPtr constructor doesn't do it
		iter->second->reference();
		return Glib::RefPtr<Gdk::Pixbuf>(iter->second);
	}

	try {
		buf = Gdk::Pixbuf::create_from_file(path, size, size);
		_icons.insert(make_pair(make_pair(path, size), buf.operator->()));
		buf->add_destroy_notify_callback(
			new IconDestroyNotification(*this, make_pair(path, size)),
			&App::icon_destroyed);
	} catch (const Glib::Error& e) {
		log().warn(Raul::fmt("Error loading icon %1%: %2%\n")
		           % path % e.what());
	}
	return buf;
}

void*
App::icon_destroyed(void* data)
{
	IconDestroyNotification* note = (IconDestroyNotification*)data;
	Icons::iterator iter = note->app._icons.find(note->key);
	if (iter != note->app._icons.end())
		note->app._icons.erase(iter);

	delete note; // allocated in App::icon_from_path

	return NULL;
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

