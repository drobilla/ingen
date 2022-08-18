/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

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

#include "ingen/Message.hpp"
#include "ingen/Properties.hpp"
#include "ingen/Resource.hpp"
#include "ingen/Status.hpp"
#include "ingen/URI.hpp"
#include "ingen/World.hpp"
#include "ingen/ingen.h"
#include "lilv/lilv.h"

#include <sigc++/signal.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace Gtk {
class AboutDialog;
class Main;
class Widget;
class Window;
} // namespace Gtk

namespace ingen {

class Atom;
class Forge;
class Interface;
class Log;
class Serialiser;
class StreamWriter;
class URIs;

namespace client {

class ClientStore;
class PortModel;
class SigClientInterface;

} // namespace client

namespace gui {

class ConnectWindow;
class GraphTreeWindow;
class MessagesWindow;
class Port;
class Style;
class ThreadedLoader;
class WindowFactory;

/** Ingen Gtk Application.
 * \ingroup GUI
 */
class INGEN_API App
{
public:
	~App();

	void error_message(const std::string& str);

	void attach(const std::shared_ptr<ingen::Interface>& client);

	void detach();

	void request_plugins_if_necessary();

	void register_callbacks();
	bool gtk_main_iteration();

	void show_about();
	bool quit(Gtk::Window* dialog_parent);

	void port_activity(Port* port);
	void activity_port_destroyed(Port* port);
	bool can_control(const client::PortModel* port) const;

	bool signal() const { return _enable_signal; }
	void enable_signals(bool b) { _enable_signal = b; }
	bool disable_signals() {
		bool old = _enable_signal;
		_enable_signal = false;
		return old;
	}

	void set_property(const URI&      subject,
	                  const URI&      key,
	                  const Atom&     value,
	                  Resource::Graph ctx = Resource::Graph::DEFAULT);

	/** Set the tooltip for a widget from its RDF documentation. */
	void set_tooltip(Gtk::Widget* widget, const LilvNode* node);

	uint32_t sample_rate() const;

	bool is_plugin() const { return _is_plugin; }
	void set_is_plugin(bool b) { _is_plugin = b; }

	ConnectWindow*   connect_window()  const { return _connect_window; }
	MessagesWindow*  messages_dialog() const { return _messages_window; }
	GraphTreeWindow* graph_tree()      const { return _graph_tree_window; }
	Style*           style()           const { return _style; }
	WindowFactory*   window_factory()  const { return _window_factory; }

	ingen::Forge&                        forge()     const { return _world.forge(); }
	std::shared_ptr<ingen::Interface>    interface() const { return _world.interface(); }
	std::shared_ptr<ingen::Interface>    client()    const { return _client; }
	std::shared_ptr<client::ClientStore> store()     const { return _store; }
	std::shared_ptr<ThreadedLoader>      loader()    const { return _loader; }

	std::shared_ptr<client::SigClientInterface> sig_client();

	std::shared_ptr<Serialiser> serialiser();

	static std::shared_ptr<App> create(ingen::World& world);

	void run();

	std::string status_text() const;

	sigc::signal<void, const std::string&> signal_status_text_changed;

	inline ingen::World& world() const { return _world; }
	inline ingen::URIs&  uris()  const { return _world.uris(); }
	inline ingen::Log&   log()   const { return _world.log(); }

protected:
	explicit App(ingen::World& world);

	void message(const ingen::Message& msg);

	bool animate();
	void response(int32_t id, ingen::Status status, const std::string& subject);

	void put(const URI&        uri,
	         const Properties& properties,
	         Resource::Graph   ctx);

	void property_change(const URI&      subject,
	                     const URI&      key,
	                     const Atom&     value,
	                     Resource::Graph ctx = Resource::Graph::DEFAULT);

	static Gtk::Main* _main;

	std::shared_ptr<ingen::Interface>    _client;
	std::shared_ptr<client::ClientStore> _store;
	std::shared_ptr<ThreadedLoader>      _loader;
	std::shared_ptr<StreamWriter>        _dumper;

	Style* _style;

	ConnectWindow*    _connect_window = nullptr;
	MessagesWindow*   _messages_window = nullptr;
	GraphTreeWindow*  _graph_tree_window = nullptr;
	Gtk::AboutDialog* _about_dialog = nullptr;
	WindowFactory*    _window_factory = nullptr;

	ingen::World& _world;

	int32_t     _sample_rate{48000};
	int32_t     _block_length{1024};
	int32_t     _n_threads{1};
	float       _mean_run_load{0.0f};
	float       _min_run_load{0.0f};
	float       _max_run_load{0.0f};
	std::string _status_text;

	using ActivityPorts = std::unordered_map<Port*, bool>;
	ActivityPorts _activity_ports;

	bool _enable_signal{true};
	bool _requested_plugins{false};
	bool _is_plugin{false};
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_APP_HPP
