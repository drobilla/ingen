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
#include <util/CountedPtr.h>
using std::string; using std::map; using std::list;
using std::cerr; using std::endl;

namespace Ingen { namespace Client {
	class PatchModel;
	class PluginModel;
	class Store;
	class SigClientInterface;
	class ModelEngineInterface;
} }
using namespace Ingen::Client;

/** \defgroup Ingenuity GTK Client
 */

/** GTK Graphical client */
namespace Ingenuity {

class PatchWindow;
class GtkObjectController;
class PatchController;
class NodeController;
class PortController;
class LoadPatchWindow;
class MessagesWindow;
class ConfigWindow;
class IngenuityObject;
class OmModule;
class OmPort;
class OmFlowCanvas;
class PatchTreeView;
class PatchTreeWindow;
class ConnectWindow;
class Configuration;
class Loader;


/** Singleton master class most everything is contained within.
 *
 * This is a horrible god-object, but it's shrinking in size as things are
 * moved out.  Hopefully it will go away entirely some day..
 *
 * \ingroup Ingenuity
 */
class App
{
public:
	~App();

	void error_message(const string& msg);

	void disconnect();
	void quit();

	void add_patch_window(PatchWindow* pw);
	void remove_patch_window(PatchWindow* pw);

	int num_open_patch_windows();

	void attach(CountedPtr<ModelEngineInterface>& engine, CountedPtr<SigClientInterface>& client);

	ConnectWindow*   connect_window()       const { return _connect_window; }
	Gtk::Dialog*     about_dialog()         const { return _about_dialog; }
	ConfigWindow*    configuration_dialog() const { return _config_window; }
	MessagesWindow*  messages_dialog()      const { return _messages_window; }
	PatchTreeWindow* patch_tree()           const { return _patch_tree_window; }
	Configuration*   configuration()        const { return _configuration; }
	Store*           store()                const { return _store; }
	Loader*          loader()               const { return _loader; }
	
	const CountedPtr<ModelEngineInterface>& engine() const { return _engine; }
	const CountedPtr<SigClientInterface>&   client() const { return _client; }

	static inline App& instance() { assert(_instance); return *_instance; }
	static void        instantiate();

protected:
	App();
	static App* _instance;

	CountedPtr<ModelEngineInterface> _engine;
	CountedPtr<SigClientInterface>   _client;
	
	Store*  _store;
	Loader* _loader;

	Configuration*    _configuration;

	list<PatchWindow*> _windows;
	
	ConnectWindow*    _connect_window;
	MessagesWindow*   _messages_window;
	PatchTreeWindow*  _patch_tree_window;
	ConfigWindow*     _config_window;
	Gtk::Dialog*      _about_dialog;

	/** Used to avoid feedback loops with (eg) process checkbutton
	 * FIXME: Maybe this should be globally implemented at the Controller level,
	 * disable all command sending while handling events to avoid feedback
	 * issues with widget event callbacks?  This same pattern is duplicated
	 * too much... */
	bool _enable_signal;
};


} // namespace Ingenuity

#endif // OMGTKAPP_H

