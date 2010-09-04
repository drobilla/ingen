/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#include "ingen-config.h"
#include "App.hpp"
#include <cassert>
#include <string>
#include <fstream>
#include <libgnomecanvasmm.h>
#include <gtk/gtkwindow.h>
#include <time.h>
#include <sys/time.h>
#include "raul/log.hpp"
#include "raul/Path.hpp"
#include "flowcanvas/Connection.hpp"
#include "module/World.hpp"
#include "engine/Engine.hpp"
#include "interface/EngineInterface.hpp"
#include "shared/runtime_paths.hpp"
#include "shared/LV2URIMap.hpp"
#include "client/ObjectModel.hpp"
#include "client/PatchModel.hpp"
#include "client/ClientStore.hpp"
#include "NodeModule.hpp"
#include "ControlPanel.hpp"
#include "SubpatchModule.hpp"
#include "LoadPluginWindow.hpp"
#include "PatchWindow.hpp"
#include "MessagesWindow.hpp"
#include "GladeFactory.hpp"
#include "PatchTreeWindow.hpp"
#include "Configuration.hpp"
#include "ConnectWindow.hpp"
#include "ThreadedLoader.hpp"
#include "WindowFactory.hpp"
#include "Port.hpp"
#ifdef HAVE_SLV2
#include "slv2/slv2.h"
#endif

using namespace std;
using namespace Raul;
using namespace Ingen::Client;

namespace Raul { class Deletable; }

namespace Ingen { namespace Client { class PluginModel; } }

namespace Ingen {
namespace GUI {

class Port;

Gtk::Main* App::_main = 0;

/// Singleton instance
App* App::_instance = 0;


App::App(Ingen::Shared::World* world)
	: _configuration(new Configuration())
	, _about_dialog(NULL)
	, _window_factory(new WindowFactory())
	, _world(world)
	, _enable_signal(true)
{
	Glib::RefPtr<Gnome::Glade::Xml> glade_xml = GladeFactory::new_glade_reference();

	Glib::set_application_name("Ingen");
	gtk_window_set_default_icon_name("ingen");

	glade_xml->get_widget_derived("connect_win", _connect_window);
	glade_xml->get_widget_derived("messages_win", _messages_window);
	glade_xml->get_widget_derived("patch_tree_win", _patch_tree_window);
	glade_xml->get_widget("about_win", _about_dialog);
	_about_dialog->property_program_name() = "Ingen";
	_about_dialog->property_logo_icon_name() = "ingen";

	PluginModel::set_rdf_world(*world->rdf_world());

#ifdef HAVE_SLV2
	PluginModel::set_slv2_world(world->slv2_world());
#endif
}


App::~App()
{
	delete _configuration;
	delete _window_factory;
}


void
App::init(Ingen::Shared::World* world)
{
	Gnome::Canvas::init();
	_main = new Gtk::Main(&world->argc(), &world->argv());

	if (!_instance)
		_instance = new App(world);

	// Load configuration settings
	_instance->configuration()->load_settings();
	_instance->configuration()->apply_settings();

	// Set default window icon
	_instance->_about_dialog->property_program_name() = "Ingen";
	_instance->_about_dialog->property_logo_icon_name() = "ingen";
	gtk_window_set_default_icon_name("ingen");

	// Set style for embedded node GUIs
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

	App::instance().connect_window()->start(world);

	// Run main iterations here until we're attached to the engine
	// (otherwise with 'ingen -egl' we'll get a bunch of notifications about load immediately
	// before even knowing about the root patch or plugins)
	while (!App::instance().connect_window()->attached())
		if (_main->iteration())
			break;
}


void
App::run()
{
	assert(_main);
	_main->run();
	info << "[GUI] Exiting" << endl;
}


void
App::attach(SharedPtr<SigClientInterface> client,
            SharedPtr<Raul::Deletable>    handle)
{
	assert( ! _client);
	assert( ! _store);
	assert( ! _loader);

	_world->engine()->register_client(client.get());

	_client = client;
	_handle = handle;
	_store  = SharedPtr<ClientStore>(new ClientStore(_world->uris(), _world->engine(), client));
	_loader = SharedPtr<ThreadedLoader>(new ThreadedLoader(_world->uris(), _world->engine()));

	_patch_tree_window->init(*_store);

	_client->signal_response_error.connect(sigc::mem_fun(this, &App::error_response));
	_client->signal_error.connect(sigc::mem_fun(this, &App::error_message));
}


void
App::detach()
{
	if (_world->engine()) {
		_window_factory->clear();
		_store->clear();

		_loader.reset();
		_store.reset();
		_client.reset();
		_handle.reset();
		_world->set_engine(SharedPtr<EngineInterface>());
	}
}


SharedPtr<Serialiser>
App::serialiser()
{
	if (!_world->serialiser())
		_world->load("ingen_serialisation");

	return _world->serialiser();
}


void
App::error_response(int32_t id, const string& str)
{
	error_message(str);
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
App::port_activity(Port* port)
{
	std::pair<ActivityPorts::iterator, bool> inserted = _activity_ports.insert(make_pair(port, false));
	if (inserted.second)
		inserted.first->second = false;

	if (port->is_output()) {
		for (Port::Connections::const_iterator i = port->connections().begin(); i != port->connections().end(); ++i) {
			const SharedPtr<Port> dst = PtrCast<Port>(i->lock()->dest().lock());
			if (dst)
				port_activity(dst.get());
		}
	}

	port->set_highlighted(true, false, true, false);
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
	for (ActivityPorts::iterator i = _activity_ports.begin(); i != _activity_ports.end() ; ) {
		ActivityPorts::iterator next = i;
		++next;

		if ((*i).second) { // saw it last time, unhighlight and pop
			(*i).first->set_highlighted(false, false, true, false);
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
		sigc::mem_fun(App::instance(), &App::gtk_main_iteration), 25, G_PRIORITY_DEFAULT);

	Glib::signal_timeout().connect(
		sigc::mem_fun(App::instance(), &App::animate), 50, G_PRIORITY_DEFAULT);
}


bool
App::gtk_main_iteration()
{
	if (!_client)
		return false;

	if (_world->local_engine()) {
		_world->local_engine()->main_iteration();
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
App::quit(Gtk::Window& dialog_parent)
{
	bool quit = true;
	if (App::instance().world()->local_engine()) {
		Gtk::MessageDialog d(dialog_parent,
			"The engine is running in this process.  Quitting will terminate Ingen."
			"\n\n" "Are you sure you want to quit?",
			true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE, true);
		d.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		d.add_button(Gtk::Stock::QUIT, Gtk::RESPONSE_CLOSE);
		quit = (d.run() == Gtk::RESPONSE_CLOSE);
	}

	if (quit)
		Gtk::Main::quit();

	return quit;
}


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
		buf->add_destroy_notify_callback(new pair<string, int>(path, size),
				&App::icon_destroyed);
	} catch (Glib::Error e) {
		warn << "Error loading icon: " << e.what() << endl;
	}
	return buf;
}


void*
App::icon_destroyed(void* data)
{
	pair<string, int>* p = static_cast<pair<string, int>*>(data);
	Icons::iterator iter = instance()._icons.find(*p);
	if (iter != instance()._icons.end())
		instance()._icons.erase(iter);

	delete p; // allocated in App::icon_from_path

	return NULL;
}


bool
App::can_control(const Shared::Port* port) const
{
	return port->is_a(PortType::CONTROL)
		|| (port->is_a(PortType::VALUE)
				&& (port->supports(uris().object_class_float32)
					|| port->supports(uris().object_class_string)));
}


} // namespace GUI
} // namespace Ingen

