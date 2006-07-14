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
#include "PatchView.h"
#include "OmModule.h"
#include "ControlPanel.h"
#include "SubpatchModule.h"
#include "OmFlowCanvas.h"
#include "GtkObjectController.h"
#include "PatchController.h"
#include "NodeController.h"
#include "PortController.h"
#include "LoadPluginWindow.h"
#include "PatchWindow.h"
#include "MessagesWindow.h"
#include "ConfigWindow.h"
#include "Controller.h"
#include "GladeFactory.h"
#include "util/Path.h"
#include "ObjectModel.h"
#include "PatchModel.h"
#include "PatchTreeWindow.h"
#include "Configuration.h"
#include "ConnectWindow.h"
#ifdef HAVE_LASH
#include "LashController.h"
#endif
using std::cerr; using std::cout; using std::endl;
using std::string;
namespace LibOmClient { class PluginModel; }
using namespace LibOmClient;

namespace OmGtk {

class OmPort;


/// Singleton instance
App* App::_instance = 0;


App::App()
: m_configuration(new Configuration()),
  m_about_dialog(NULL),
  m_enable_signal(true)
{
	Glib::RefPtr<Gnome::Glade::Xml> glade_xml = GladeFactory::new_glade_reference();

	glade_xml->get_widget_derived("connect_win", m_connect_window);
	//glade_xml->get_widget_derived("new_patch_win", m_new_patch_window);
	//glade_xml->get_widget_derived("load_patch_win", m_load_patch_window);
	glade_xml->get_widget_derived("config_win", m_config_window);
	glade_xml->get_widget_derived("patch_tree_win", m_patch_tree_window);
//	glade_xml->get_widget_derived("main_patches_treeview", m_objects_treeview);
	glade_xml->get_widget("about_win", m_about_dialog);
	
	m_config_window->configuration(m_configuration);

	glade_xml->get_widget_derived("messages_win", m_messages_window);
}


App::~App()
{
}


void
App::error_message(const string& str)
{
	m_messages_window->post(str);
	m_messages_window->show();
	m_messages_window->raise();
}


/*
bool
App::idle_callback()
{	
	m_client_hooks->process_events();

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
		= new Gtk::FileChooserDialog(*m_main_window, "Load Session", Gtk::FILE_CHOOSER_ACTION_OPEN);
	
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
	Gtk::FileChooserDialog dialog(*m_main_window, "Save Session", Gtk::FILE_CHOOSER_ACTION_SAVE);
	
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
			Gtk::MessageDialog confirm_dialog(*m_main_window,
				msg, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
			if (confirm_dialog.run() == Gtk::RESPONSE_YES)
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
	m_windows.push_back(pw);
}


void
App::remove_patch_window(PatchWindow* pw)
{
	m_windows.erase(find(m_windows.begin(), m_windows.end(), pw));
}


/** Returns the number of Patch windows currently visible.
 */
int
App::num_open_patch_windows()
{
	int ret = 0;
	for (list<PatchWindow*>::iterator i = m_windows.begin(); i != m_windows.end(); ++i)
		if ((*i)->is_visible())
			++ret;

	return ret;
}

void
App::quit()
{
	Gtk::Main::quit();
}


} // namespace OmGtk

