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

#include "config.h"
#include "App.hpp"
#include <cassert>
#include <string>
#include <fstream>
#include <libgnomecanvasmm.h>
#include <time.h>
#include <sys/time.h>
#include "raul/Path.hpp"
#include "flowcanvas/Connection.hpp"
#include "module/global.hpp"
#include "module/Module.hpp"
#include "module/World.hpp"
#include "interface/EngineInterface.hpp"
#include "serialisation/serialisation.hpp"
#include "client/ObjectModel.hpp"
#include "client/PatchModel.hpp"
#include "client/ClientStore.hpp"
#include "engine/Engine.hpp"
#include "shared/runtime_paths.hpp"
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
using namespace Ingen::Client;

namespace Ingen { namespace Client { class PluginModel; } }

namespace Ingen {
namespace GUI {

class Port;


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

	glade_xml->get_widget_derived("connect_win", _connect_window);
	glade_xml->get_widget_derived("messages_win", _messages_window);
	glade_xml->get_widget_derived("patch_tree_win", _patch_tree_window);
	glade_xml->get_widget("about_win", _about_dialog);
	_about_dialog->property_program_name() = "Ingen";

	PluginModel::set_rdf_world(*world->rdf_world);

#ifdef HAVE_SLV2
	PluginModel::set_slv2_world(world->slv2_world);
#endif
}


App::~App()
{
}

void
App::run(int argc, char** argv, Ingen::Shared::World* world)
{
	Gnome::Canvas::init();
	Gtk::Main main(argc, argv);

	if (!_instance)
		_instance = new App(world);
	
	// Load configuration settings
	_instance->configuration()->load_settings();
	_instance->configuration()->apply_settings();
	
	// Set default window icon
	const Glib::ustring icon_path = Shared::data_file_path("ingen.svg");
	try {
		if (Glib::file_test(icon_path, Glib::FILE_TEST_EXISTS))
			Gtk::Window::set_default_icon_from_file(icon_path);
	} catch (Gdk::PixbufError err) {
		cerr << "Unable to load window icon " << icon_path << ": " << err.what() << endl;
	}
	
	// Set style for embedded node GUIs
	const string rc_style =
	  "style \"ingen_embedded_node_gui_style\" {"
	  " bg[NORMAL] = \"#212222\""
	  " bg[ACTIVE] = \"#505050\""
	  " bg[PRELIGHT] = \"#525454\""
	  " bg[SELECTED] = \"#99A0A0\""
	  " bg[INSENSITIVE] = \"#F03030\""
	  " fg[NORMAL] = \"#FFFFFF\""
	  " fg[ACTIVE] = \"#FFFFFF\""
	  " fg[PRELIGHT] = \"#FFFFFF\""
	  " fg[SELECTED] = \"#FFFFFF\""
	  " fg[INSENSITIVE] = \"#FFFFFF\""
	  "}\n"
	  "widget \"*ingen_embedded_node_gui_container*\" style \"ingen_embedded_node_gui_style\"\n";
 	
	Gtk::RC::parse_string(rc_style);
	
	App::instance().connect_window()->start(world);
	
	main.run();

	cout << "Gtk exiting." << endl;
}


void
App::attach(SharedPtr<SigClientInterface> client,
            SharedPtr<Raul::Deletable>    handle)
{
	assert( ! _client);
	assert( ! _store);
	assert( ! _loader);

	_world->engine->register_client(client.get());
	
	_client = client;
	_handle = handle;
	_store = SharedPtr<ClientStore>(new ClientStore(_world->engine, client));
	_loader = SharedPtr<ThreadedLoader>(new ThreadedLoader(_world->engine));

	_patch_tree_window->init(*_store);
	
	_client->signal_response_error.connect(sigc::mem_fun(this, &App::error_response));
	_client->signal_error.connect(sigc::mem_fun(this, &App::error_message));
}


void
App::detach()
{
	if (_world->engine) {
		_window_factory->clear();
		_store->clear();

		_loader.reset();
		_store.reset();
		_client.reset();
		_handle.reset();
		_world->engine.reset();
	}
}
	

const SharedPtr<Serialiser>&
App::serialiser()
{
	if (!_serialiser) {
		if (!_world->serialisation_module)
			_world->serialisation_module = Ingen::Shared::load_module("ingen_serialisation");

		if (_world->serialisation_module)
			_serialiser = SharedPtr<Serialiser>(Ingen::Serialisation::new_serialiser(_world, _store));

		if (!_serialiser)
			cerr << "WARNING: Failed to load ingen_serialisation module, save disabled." << endl;
	}
	return _serialiser;
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


#if 0
App::event_load_session()
{
	Gtk::FileChooserDialog* dialog
		= new Gtk::FileChooserDialog(*_main_window, "Load Session", Gtk::FILE_CHOOSER_ACTION_OPEN);
	
	dialog->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog->add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);	
	int result = dialog->run();
	string filename = dialog->get_filename();
	delete dialog;

	cout << result << endl;
	
	assert(result == Gtk::RESPONSE_OK || result == Gtk::RESPONSE_CANCEL || result == Gtk::RESPONSE_NONE);
	
	if (result == Gtk::RESPONSE_OK)
		//configuration->load_session(filename);
		_controller->load_session(filename);
}


void
App::event_save_session_as()
{
	Gtk::FileChooserDialog dialog(*_main_window, "Save Session", Gtk::FILE_CHOOSER_ACTION_SAVE);
	
	/*
	Gtk::VBox* box = dialog.get_vbox();
	Gtk::Label warning("Warning:  Recursively saving will overwrite any subpatch files \
		without confirmation.");
	box->pack_start(warning, false, false, 2);
	Gtk::CheckButton recursive_checkbutton("Recursively save all subpatches");
	box->pack_start(recursive_checkbutton, false, false, 0);
	recursive_checkbutton.show();
	*/		
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);	
	
	int result = dialog.run();
	//bool recursive = recursive_checkbutton.get_active();
	
	assert(result == Gtk::RESPONSE_OK || result == Gtk::RESPONSE_CANCEL || result == Gtk::RESPONSE_NONE);
	
	if (result == Gtk::RESPONSE_OK) {	
		string filename = dialog.get_filename();
		if (filename.length() < 11 || filename.substr(filename.length()-10) != ".omsession")
			filename += ".omsession";
			
		bool confirm = false;
		std::fstream fin;
		fin.open(filename.c_str(), std::ios::in);
		if (fin.is_open()) {  // File exists
			string msg = "File already exists!  Are you sure you want to overwrite ";
			msg += filename + "?";
			Gtk::MessageDialog confir_dialog(*_main_window,
				msg, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
			if (confir_dialog.run() == Gtk::RESPONSE_YES)
				confirm = true;
			else
				confirm = false;
		} else {  // File doesn't exist
			confirm = true;
		}
		fin.close();
		
		if (confirm) {
			_controller->save_session(filename);
		}
	}
}
#endif


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

	if (_world->local_engine) {
		_world->local_engine->main_iteration();
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


void
App::quit()
{
	Gtk::Main::quit();
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
		cerr << "Loaded icon " << path << " with size " << size << endl;
	} catch (Glib::Error e) {
		cerr << "Error loading icon: " << e.what() << endl;
	}
	return buf;
}


void*
App::icon_destroyed(void* data)
{
	pair<string, int>* p = static_cast<pair<string, int>*>(data);
	cerr << "Destroyed icon " << p->first << " with size " << p->second << endl;
	Icons::iterator iter = instance()._icons.find(*p);
	if (iter != instance()._icons.end())
		instance()._icons.erase(iter);
	
	delete p; // allocated in App::icon_from_path
	
	return NULL;
}


} // namespace GUI
} // namespace Ingen

