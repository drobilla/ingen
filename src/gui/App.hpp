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

#ifndef INGEN_GUI_APP_HPP
#define INGEN_GUI_APP_HPP

#include <cassert>
#include <string>
#include <map>
#include <utility>

#include <gtkmm/aboutdialog.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>

#include "ingen/Status.hpp"
#include "ingen/World.hpp"
#include "raul/Atom.hpp"
#include "raul/Deletable.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/URI.hpp"

namespace Ingen {
	class Interface;
	class Port;
	class World;
	namespace Client {
		class ClientStore;
		class PatchModel;
		class PluginModel;
		class PortModel;
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

/** Ingen Gtk Application.
 * \ingroup GUI
 */
class App
{
public:
	~App();

	void error_message(const std::string& msg);

	void attach(SharedPtr<Client::SigClientInterface> client);

	void detach();

	void register_callbacks();
	bool gtk_main_iteration();

	void show_about();
	bool quit(Gtk::Window* dialog_parent);

	void port_activity(Port* port);
	void activity_port_destroyed(Port* port);
	bool can_control(const Client::PortModel* port) const;

	bool signal() const { return _enable_signal; }
	void enable_signals(bool b) { _enable_signal = b; }
	bool disable_signals() {
		bool old = _enable_signal;
		_enable_signal = false;
		return old;
	}

	uint32_t sample_rate() const;

	ConnectWindow*   connect_window()  const { return _connect_window; }
	MessagesWindow*  messages_dialog() const { return _messages_window; }
	PatchTreeWindow* patch_tree()      const { return _patch_tree_window; }
	Configuration*   configuration()   const { return _configuration; }
	WindowFactory*   window_factory()  const { return _window_factory; }

	Glib::RefPtr<Gdk::Pixbuf> icon_from_path(const std::string& path, int size);

	Ingen::Forge&                         forge()     const { return _world->forge(); }
	SharedPtr<Ingen::Interface>           interface() const { return _world->interface(); }
	SharedPtr<Client::SigClientInterface> client()    const { return _client; }
	SharedPtr<Client::ClientStore>        store()     const { return _store; }
	SharedPtr<ThreadedLoader>             loader()    const { return _loader; }

	SharedPtr<Serialisation::Serialiser> serialiser();

	static SharedPtr<App> create(Ingen::World* world);
	void run();

	inline Ingen::World* world() const { return _world; }
	inline Ingen::URIs&  uris()  const { return _world->uris(); }

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

	explicit App(Ingen::World* world);

	bool animate();
	void response(int32_t id, Ingen::Status status, const std::string& subject);

	void property_change(const Raul::URI&  subject,
	                     const Raul::URI&  key,
	                     const Raul::Atom& value);

	static void* icon_destroyed(void* data);

	static Gtk::Main* _main;

	SharedPtr<Client::SigClientInterface> _client;
	SharedPtr<Client::ClientStore>        _store;
	SharedPtr<ThreadedLoader>             _loader;

	Configuration*    _configuration;

	ConnectWindow*    _connect_window;
	MessagesWindow*   _messages_window;
	PatchTreeWindow*  _patch_tree_window;
	Gtk::AboutDialog* _about_dialog;
	WindowFactory*    _window_factory;

	Ingen::World* _world;

	uint32_t _sample_rate;

	typedef std::map<Port*, bool> ActivityPorts;
	ActivityPorts _activity_ports;

	bool _enable_signal;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_APP_HPP

