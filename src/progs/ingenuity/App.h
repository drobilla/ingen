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

#ifndef OMGTKAPP_H
#define OMGTKAPP_H

#include <cassert>
#include <string>
#include <map>
#include <list>
#include <iostream>
#include <libgnomecanvasmm.h>
#include <gtkmm.h>
#include <libglademm.h>
using std::string; using std::map; using std::list;
using std::cerr; using std::endl;

namespace LibOmClient { class PatchModel; class PluginModel; }
using namespace LibOmClient;

/** \defgroup OmGtk GTK Client
 */

namespace OmGtk {

class PatchWindow;
class GtkObjectController;
class PatchController;
class NodeController;
class PortController;
class LoadPatchWindow;
class MessagesWindow;
class ConfigWindow;
class OmGtkObject;
class OmModule;
class OmPort;
class OmFlowCanvas;
class PatchTreeView;
class PatchTreeWindow;
class ConnectWindow;
class Configuration;


/** Singleton master class most everything is contained within.
 *
 * This is a horrible god-object, but it's shrinking in size as things are
 * moved out.  Hopefully it will go away entirely some day..
 *
 * \ingroup OmGtk
 */
class App
{
public:
	~App();

	void error_message(const string& msg);

	void quit();

	void add_patch_window(PatchWindow* pw);
	void remove_patch_window(PatchWindow* pw);

	int  num_open_patch_windows();

	ConnectWindow*   connect_window()       const { return m_connect_window; }
	Gtk::Dialog*     about_dialog()         const { return m_about_dialog; }
	ConfigWindow*    configuration_dialog() const { return m_config_window; }
	MessagesWindow*  messages_dialog()      const { return m_messages_window; }
	PatchTreeWindow* patch_tree()           const { return m_patch_tree_window; }
	Configuration*   configuration()        const { return m_configuration; }

	static void        instantiate() { if (!_instance) _instance = new App(); }
	static inline App& instance()    { assert(_instance); return *_instance; }

protected:
	App();
	static App* _instance;

	//bool connect_callback();
	//bool idle_callback();

	Configuration*    m_configuration;

	list<PatchWindow*> m_windows;
	
	ConnectWindow*    m_connect_window;
	MessagesWindow*   m_messages_window;
	PatchTreeWindow*  m_patch_tree_window;
	ConfigWindow*     m_config_window;
	Gtk::Dialog*      m_about_dialog;
	Gtk::Button*      m_engine_error_close_button;

	/** Used to avoid feedback loops with (eg) process checkbutton
	 * FIXME: Maybe this should be globally implemented at the Controller level,
	 * disable all command sending while handling events to avoid feedback
	 * issues with widget event callbacks?  This same pattern is duplicated
	 * too much... */
	bool m_enable_signal;
};


} // namespace OmGtk

#endif // OMGTKAPP_H

