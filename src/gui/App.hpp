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
#include <map>
#include <string>
#include <utility>

#include <gtkmm/aboutdialog.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>

#include "ingen/Atom.hpp"
#include "ingen/Status.hpp"
#include "ingen/World.hpp"
#include "ingen/types.hpp"
#include "raul/Deletable.hpp"
#include "raul/URI.hpp"

namespace Ingen {
class Interface;
class Log;
class Port;
class Serialiser;
class World;
namespace Client {
class ClientStore;
class GraphModel;
class PluginModel;
class PortModel;
class SigClientInterface;
}
}

namespace Ingen {

namespace GUI {

class ConnectWindow;
class GraphCanvas;
class GraphTreeView;
class GraphTreeWindow;
class MessagesWindow;
class Port;
class Style;
class ThreadedLoader;
class WindowFactory;

/** Ingen Gtk Application.
 * \ingroup GUI
 */
class App
{
public:
	~App();

	void error_message(const std::string& msg);

	void attach(SPtr<Client::SigClientInterface> client);

	void detach();

	void request_plugins_if_necessary();

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

	void set_property(const Raul::URI& subject,
	                  const Raul::URI& key,
	                  const Atom&      value);

	uint32_t sample_rate() const;

	ConnectWindow*   connect_window()  const { return _connect_window; }
	MessagesWindow*  messages_dialog() const { return _messages_window; }
	GraphTreeWindow* graph_tree()      const { return _graph_tree_window; }
	Style*           style()           const { return _style; }
	WindowFactory*   window_factory()  const { return _window_factory; }

	Ingen::Forge&                    forge()     const { return _world->forge(); }
	SPtr<Ingen::Interface>           interface() const { return _world->interface(); }
	SPtr<Client::SigClientInterface> client()    const { return _client; }
	SPtr<Client::ClientStore>        store()     const { return _store; }
	SPtr<ThreadedLoader>             loader()    const { return _loader; }

	SPtr<Serialiser> serialiser();

	static SPtr<App> create(Ingen::World* world);
	void run();

	inline Ingen::World* world() const { return _world; }
	inline Ingen::URIs&  uris()  const { return _world->uris(); }
	inline Ingen::Log&   log()   const { return _world->log(); }

protected:
	explicit App(Ingen::World* world);

	bool animate();
	void response(int32_t id, Ingen::Status status, const std::string& subject);

	void property_change(const Raul::URI& subject,
	                     const Raul::URI& key,
	                     const Atom&      value);

	static Gtk::Main* _main;

	SPtr<Client::SigClientInterface> _client;
	SPtr<Client::ClientStore>        _store;
	SPtr<ThreadedLoader>             _loader;

	Style* _style;

	ConnectWindow*    _connect_window;
	MessagesWindow*   _messages_window;
	GraphTreeWindow*  _graph_tree_window;
	Gtk::AboutDialog* _about_dialog;
	WindowFactory*    _window_factory;

	Ingen::World* _world;

	uint32_t _sample_rate;

	typedef std::map<Port*, bool> ActivityPorts;
	ActivityPorts _activity_ports;

	bool _enable_signal;
	bool _requested_plugins;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_APP_HPP
