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

#include CONFIG_H_PATH
#include "module/module.h"
#include "App.hpp"
#include <cassert>
#include <string>
#include <fstream>
#include <libgnomecanvasmm.h>
#include <time.h>
#include <sys/time.h>
#include <raul/Path.hpp>
#include "interface/EngineInterface.hpp"
#include "client/ObjectModel.hpp"
#include "client/PatchModel.hpp"
#include "client/Store.hpp"
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
/*#ifdef HAVE_LASH
#include "LashController.hpp"
#endif*/
#ifdef HAVE_SLV2
#include <slv2/slv2.h>
#endif
using std::cerr; using std::cout; using std::endl;
using std::string;
namespace Ingen { namespace Client { class PluginModel; } }
using namespace Ingen::Client;

namespace Ingen {
namespace GUI {

class Port;


/// Singleton instance
App* App::_instance = 0;


App::App(Ingen::Shared::World* world)
: _configuration(new Configuration()),
  _about_dialog(NULL),
  _window_factory(new WindowFactory()),
  _world(world),
  _enable_signal(true)
{
	Glib::RefPtr<Gnome::Glade::Xml> glade_xml = GladeFactory::new_glade_reference();

	glade_xml->get_widget_derived("connect_win", _connect_window);
	glade_xml->get_widget_derived("messages_win", _messages_window);
	glade_xml->get_widget_derived("patch_tree_win", _patch_tree_window);
	glade_xml->get_widget("about_win", _about_dialog);

	Raul::RDF::World& rdf_world = *world->rdf_world;

	rdf_world.add_prefix("xsd", "http://www.w3.org/2001/XMLSchema#");
	rdf_world.add_prefix("ingen", "http://drobilla.net/ns/ingen#");
	rdf_world.add_prefix("ingenuity", "http://drobilla.net/ns/ingenuity#");
	rdf_world.add_prefix("lv2", "http://lv2plug.in/ns/lv2core#");
	rdf_world.add_prefix("rdfs", "http://www.w3.org/2000/01/rdf-schema#");
	rdf_world.add_prefix("doap", "http://usefulinc.com/ns/doap#");
	
	PluginModel::set_rdf_world(rdf_world);

#ifdef HAVE_SLV2
	PluginModel::set_slv2_world(world->slv2_world);
#endif
}


App::~App()
{
}

void
App::run(int argc, char** argv,
		Ingen::Shared::World* world,
		SharedPtr<Engine> engine,
		SharedPtr<Shared::EngineInterface> interface)
{
	Gnome::Canvas::init();
	Gtk::Main main(argc, argv);

	if (!_instance)
		_instance = new App(world);
	
	/* Load settings */
	_instance->configuration()->load_settings();
	_instance->configuration()->apply_settings();
	
	const Glib::ustring icon_path = PKGDATADIR "/ingen.svg";
	try {
		if (Glib::file_test(icon_path, Glib::FILE_TEST_EXISTS))
			Gtk::Window::set_default_icon_from_file(icon_path);
	} catch (Gdk::PixbufError err) {
		cerr << "Unable to load window icon " << icon_path << ": " << err.what() << endl;
	}
	
	App::instance().connect_window()->start(engine, interface);
	
	main.run();

	cout << "Gtk exiting." << endl;
}


void
App::attach(SharedPtr<EngineInterface> engine, SharedPtr<SigClientInterface> client)
{
	assert( ! _engine);
	assert( ! _client);
	assert( ! _store);
	assert( ! _loader);

	engine->register_client(client.get());
	
	_engine = engine;
	_client = client;
	_store = SharedPtr<Store>(new Store(engine, client));
	_loader = SharedPtr<ThreadedLoader>(new ThreadedLoader(engine));

	_patch_tree_window->init(*_store);
}


void
App::detach()
{
	if (_engine) {
		_window_factory->clear();
		_store->clear();

		_loader.reset();
		_store.reset();
		_client.reset();
		_engine.reset();
	}
}


void
App::error_message(const string& str)
{
	_messages_window->post(str);
	_messages_window->show();
	_messages_window->raise();
}


/*
bool
App::idle_callback()
{	
	_client_hooks->process_events();

#ifdef HAVE_LASH
	//if (lash_controller->enabled())
	//	lash_controller->process_events();
#endif
	
	return true;
}
*/


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
App::quit()
{
	Gtk::Main::quit();
}


} // namespace GUI
} // namespace Ingen

