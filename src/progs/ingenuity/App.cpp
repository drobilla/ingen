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

#include "config.h"
#include "App.h"
#include <cassert>
#include <string>
#include <fstream>
#include <libgnomecanvasmm.h>
#include <time.h>
#include <sys/time.h>
#include "NodeModule.h"
#include "ControlPanel.h"
#include "SubpatchModule.h"
#include "LoadPluginWindow.h"
#include "PatchWindow.h"
#include "MessagesWindow.h"
#include "ConfigWindow.h"
#include "GladeFactory.h"
#include "util/Path.h"
#include "ObjectModel.h"
#include "PatchModel.h"
#include "PatchTreeWindow.h"
#include "Configuration.h"
#include "ConnectWindow.h"
#include "Store.h"
#include "Loader.h"
#include "WindowFactory.h"
#ifdef HAVE_LASH
#include "LashController.h"
#endif
using std::cerr; using std::cout; using std::endl;
using std::string;
namespace Ingen { namespace Client { class PluginModel; } }
using namespace Ingen::Client;

namespace Ingenuity {

class Port;


/// Singleton instance
App* App::_instance = 0;


App::App()
: _store(NULL),
  _loader(NULL),
  _configuration(new Configuration()),
  _about_dialog(NULL),
  _window_factory(new WindowFactory()),
  _enable_signal(true)
{
	Glib::RefPtr<Gnome::Glade::Xml> glade_xml = GladeFactory::new_glade_reference();

	glade_xml->get_widget_derived("connect_win", _connect_window);
	glade_xml->get_widget_derived("messages_win", _messages_window);
	glade_xml->get_widget_derived("patch_tree_win", _patch_tree_window);
	glade_xml->get_widget_derived("config_win", _config_window);
	glade_xml->get_widget("about_win", _about_dialog);

	
	_config_window->configuration(_configuration);
}


App::~App()
{
}

void
App::instantiate()
{
	if (!_instance)
		_instance = new App();
}


void
App::attach(const CountedPtr<ModelEngineInterface>& engine, const CountedPtr<SigClientInterface>& client)
{
	assert( ! _engine);
	assert( ! _client);
	assert( ! _store);
	assert( ! _loader);
	
	_engine = engine;
	_client = client;
	_store = new Store(client);
	_loader = new Loader(engine);
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
			Gtk::MessageDialog confir_dialog(*m_main_window,
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
App::add_patch_window(PatchWindow* pw)
{
	_windows.push_back(pw);
}


void
App::remove_patch_window(PatchWindow* pw)
{
	_windows.erase(find(_windows.begin(), _windows.end(), pw));
}


/** Returns the number of Patch windows currently visible.
 */
int
App::num_open_patch_windows()
{
	int ret = 0;
	for (list<PatchWindow*>::iterator i = _windows.begin(); i != _windows.end(); ++i)
		if ((*i)->is_visible())
			++ret;

	return ret;
}


void
App::disconnect()
{
	// FIXME: this is pretty gross.. figure out the death situation better
	
	list<PatchWindow*> windows = _windows; // make a copy

	for (list<PatchWindow*>::iterator i = windows.begin(); i != windows.end(); ++i)
		delete (*i);

	_store->clear();

	// PatchWindow destructor removes them from the list
	assert(_windows.size() == 0);
}


void
App::quit()
{
	Gtk::Main::quit();
}


} // namespace Ingenuity

