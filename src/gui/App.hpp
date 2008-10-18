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

#ifndef INGEN_APP_HPP
#define INGEN_APP_HPP

#include <cassert>
#include <string>
#include <map>
#include <utility>
#include <iostream>
#include <libgnomecanvasmm.h>
#include <gtkmm.h>
#include <libglademm.h>
#include "raul/SharedPtr.hpp"
#include "raul/Deletable.hpp"
#include <module/World.hpp>

namespace Ingen { 
	class Engine;
	namespace Shared {
		class EngineInterface;
		class ClientInterface;
		class World;
	}
	namespace Client {
		class PatchModel;
		class PluginModel;
		class ClientStore;
		class SigClientInterface;
	}
	namespace Serialisation {
		class Serialiser;
	}
}

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

	void error_message(const std::string& msg);

	void attach(SharedPtr<Client::SigClientInterface> client,
	            SharedPtr<Raul::Deletable>            handle=SharedPtr<Raul::Deletable>());

	void detach();
	
	void register_callbacks();
	bool gtk_main_iteration();

	void show_about();
	void quit();
	
	void port_activity(Port* port);
	void activity_port_destroyed(Port* port);

	bool signal() const { return _enable_signal; }
	bool disable_signals()  { bool old = _enable_signal; _enable_signal = false; return old; }
	void enable_signals(bool b) { _enable_signal = b; }

	ConnectWindow*     connect_window()  const { return _connect_window; }
	MessagesWindow*    messages_dialog() const { return _messages_window; }
	PatchTreeWindow*   patch_tree()      const { return _patch_tree_window; }
	Configuration*     configuration()   const { return _configuration; }
	WindowFactory*     window_factory()  const { return _window_factory; }
	
	Glib::RefPtr<Gdk::Pixbuf>            icon_from_path(const std::string& path, int size);

	const SharedPtr<Shared::EngineInterface>&    engine() const { return _world->engine; }
	const SharedPtr<Client::SigClientInterface>& client() const { return _client; }
	const SharedPtr<Client::ClientStore>&        store()  const { return _store; }
	const SharedPtr<ThreadedLoader>&             loader() const { return _loader; }
	
	const SharedPtr<Serialisation::Serialiser>& serialiser();
	
	static inline App& instance() { assert(_instance); return *_instance; }

	static void run(int argc, char** argv, Ingen::Shared::World* world);

	Ingen::Shared::World* world() { return _world; }

protected:

	/** This is needed for the icon map. */
	template <typename A, typename B>
	struct LexicalCompare {
		bool operator()(const std::pair<A, B>& p1, const std::pair<A, B>& p2) {
			return (p1.first < p2.first) || 
				((p1.first == p2.first) && (p1.second < p2.second));
		}
	};
	
	typedef std::map< std::pair<std::string, int>,
	                  Gdk::Pixbuf*,
	                  LexicalCompare<std::string, int> > Icons;
	Icons _icons;
	
	App(Ingen::Shared::World* world);
	
	bool animate();
	void error_response(int32_t id, const std::string& str);
	
	static void* icon_destroyed(void* data);
	
	static App* _instance;
	
	SharedPtr<Client::SigClientInterface> _client;
	SharedPtr<Raul::Deletable>            _handle;
	SharedPtr<Client::ClientStore>        _store;
	SharedPtr<Serialisation::Serialiser>  _serialiser;
	SharedPtr<ThreadedLoader>             _loader;

	Configuration*    _configuration;

	ConnectWindow*    _connect_window;
	MessagesWindow*   _messages_window;
	PatchTreeWindow*  _patch_tree_window;
	Gtk::AboutDialog* _about_dialog;
	WindowFactory*    _window_factory;
	
	Ingen::Shared::World* _world;

	typedef std::map<Port*, bool> ActivityPorts;
	ActivityPorts _activity_ports;

	bool _enable_signal;
};


} // namespace GUI
} // namespace Ingen

#endif // INGEN_APP_HPP

