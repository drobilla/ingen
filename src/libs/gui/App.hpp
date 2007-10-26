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

#ifndef APP_H
#define APP_H

#include <cassert>
#include <string>
#include <map>
#include <list>
#include <iostream>
#include <libgnomecanvasmm.h>
#include <gtkmm.h>
#include <libglademm.h>
#include <raul/RDFWorld.hpp>
#include <raul/SharedPtr.hpp>

using namespace std;

namespace Ingen { 
	class Engine;
	namespace Shared {
		class EngineInterface;
		class World;
	}
	namespace Client {
		class PatchModel;
		class PluginModel;
		class Store;
		class SigClientInterface;
	}
	namespace Serialisation {
		class Serialiser;
	}
}

using namespace Ingen::Shared;
using namespace Ingen::Serialisation;
using namespace Ingen::Client;

/** \defgroup GUI GTK GUI
 */

namespace Ingen {
namespace GUI {

class MessagesWindow;
class PatchCanvas;
class PatchTreeView;
class PatchTreeWindow;
class ConnectWindow;
class Configuration;
class ThreadedLoader;
class WindowFactory;
class Port;


/** Singleton master class most everything is contained within.
 *
 * This is a horrible god-object, but it's shrinking in size as things are
 * moved out.  Hopefully it will go away entirely some day..
 *
 * \ingroup GUI
 */
class App
{
public:
	~App();

	void error_message(const string& msg);

	void attach(SharedPtr<EngineInterface>    engine,
	            SharedPtr<SigClientInterface> client);
	
	void detach();
	
	bool gtk_main_iteration();
	void quit();

	void port_activity(Port* port);

	ConnectWindow*     connect_window()  const { return _connect_window; }
	Gtk::AboutDialog*  about_dialog()    const { return _about_dialog; }
	MessagesWindow*    messages_dialog() const { return _messages_window; }
	PatchTreeWindow*   patch_tree()      const { return _patch_tree_window; }
	Configuration*     configuration()   const { return _configuration; }
	WindowFactory*     window_factory()  const { return _window_factory; }
	
	Glib::RefPtr<Gdk::Pixbuf>            icon_from_path(const string& path, int size);

	const SharedPtr<EngineInterface>&    engine()     const { return _engine; }
	const SharedPtr<SigClientInterface>& client()     const { return _client; }
	const SharedPtr<Store>&              store()      const { return _store; }
	const SharedPtr<ThreadedLoader>&     loader()     const { return _loader; }
	const SharedPtr<Serialiser>&         serialiser() const { return _serialiser; }
	
	SharedPtr<Glib::Module> serialisation_module() { return _serialisation_module; }

	static inline App& instance() { assert(_instance); return *_instance; }

	static void run(int argc, char** argv,
			Ingen::Shared::World* world,
			SharedPtr<Ingen::Engine> engine,
			SharedPtr<Shared::EngineInterface> interface);

	Ingen::Shared::World* world() { return _world; }

protected:

	/** This is needed for the icon map. */
	template <typename A, typename B>
	struct LexicalCompare {
		bool operator()(const pair<A, B>& p1, const pair<A, B>& p2) {
			return (p1.first < p2.first) || 
				((p1.first == p2.first) && (p1.second < p2.second));
		}
	};
	
	typedef map<pair<string, int>, Gdk::Pixbuf*, LexicalCompare<string, int> > IconMap;
	IconMap _icons;
	
	App(Ingen::Shared::World* world);
	
	bool animate();
	
	static void* icon_destroyed(void* data);
	
	static App* _instance;
	
	SharedPtr<Glib::Module> _serialisation_module;
	
	SharedPtr<EngineInterface>    _engine;
	SharedPtr<SigClientInterface> _client;
	SharedPtr<Store>              _store;
	SharedPtr<ThreadedLoader>     _loader;
	SharedPtr<Serialiser>         _serialiser;

	Configuration*    _configuration;

	ConnectWindow*    _connect_window;
	MessagesWindow*   _messages_window;
	PatchTreeWindow*  _patch_tree_window;
	Gtk::AboutDialog* _about_dialog;
	WindowFactory*    _window_factory;
	
	Ingen::Shared::World* _world;

	/// <Port, whether it has been seen in gtk callback yet>
	typedef std::map<Port*, bool> ActivityPorts;
	ActivityPorts _activity_ports;

	/** Used to avoid feedback loops with (eg) process checkbutton
	 * FIXME: This should probably be implemented globally:
	 * disable all command sending while handling events to avoid feedback
	 * issues with widget event callbacks?  This same pattern is duplicated
	 * too much... */
	bool _enable_signal;
};


} // namespace GUI
} // namespace Ingen

#endif // APP_H

