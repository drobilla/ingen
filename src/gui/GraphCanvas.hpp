/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_GRAPHCANVAS_HPP
#define INGEN_GUI_GRAPHCANVAS_HPP

#include <ganv/Canvas.hpp>
#include <ingen/Properties.hpp>
#include <ingen/Resource.hpp>
#include <ingen/URI.hpp>
#include <lilv/lilv.h>
#include <raul/Path.hpp>

#include <gdk/gdk.h>

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

namespace Ganv {
class Module;
class Node;
class Port;
} // namespace Ganv

namespace Gtk {
class CheckMenuItem;
class Menu;
class MenuItem;
} // namespace Gtk

namespace ingen {

namespace client {
class ArcModel;
class BlockModel;
class GraphModel;
class ObjectModel;
class PluginModel;
class PortModel;
} // namespace client

namespace gui {

class App;
class PluginMenu;

/** Graph canvas widget.
 *
 * \ingroup GUI
 */
class GraphCanvas : public Ganv::Canvas
{
public:
	GraphCanvas(App&                                      app,
	            std::shared_ptr<const client::GraphModel> graph,
	            int                                       width,
	            int                                       height);

	~GraphCanvas() override = default;

	App& app() { return _app; }

	void build();
	void show_human_names(bool b);
	void show_port_names(bool b);
	bool show_port_names() const { return _show_port_names; }

	void add_plugin(const std::shared_ptr<client::PluginModel>& p);
	void remove_plugin(const URI& uri);
	void add_block(const std::shared_ptr<const client::BlockModel>& bm);
	void remove_block(const std::shared_ptr<const client::BlockModel>& bm);
	void add_port(const std::shared_ptr<const client::PortModel>& pm);
	void remove_port(const std::shared_ptr<const client::PortModel>& pm);
	void connection(const std::shared_ptr<const client::ArcModel>& arc);
	void disconnection(const std::shared_ptr<const client::ArcModel>& arc);

	void clear_selection() override;
	void destroy_selection();
	void copy_selection();
	void paste();

	void show_menu(bool position, unsigned button, uint32_t time);

	bool on_event(GdkEvent* event);

private:
	enum class ControlType { NUMBER, BUTTON };
	void generate_port_name(
		const std::string& sym_base,  std::string& symbol,
		const std::string& name_base, std::string& name);

	void menu_add_port(const std::string& sym_base,
	                   const std::string& name_base,
	                   const URI&         type,
	                   bool               is_output);

	void menu_load_plugin();
	void menu_new_graph();
	void menu_load_graph();
	void menu_properties();
	void load_plugin(const std::weak_ptr<client::PluginModel>& weak_plugin);

	void build_menus();

	void auto_menu_position(int& x, int& y, bool& push_in);

	using LV2Children = std::multimap<const std::string, const LilvPluginClass*>;

	Properties get_initial_data(Resource::Graph ctx=Resource::Graph::DEFAULT);

	Ganv::Port* get_port_view(const std::shared_ptr<client::PortModel>& port);

	void connect(Ganv::Node* tail,
	             Ganv::Node* head);

	void disconnect(Ganv::Node* tail,
	                Ganv::Node* head);

	App&                                      _app;
	std::shared_ptr<const client::GraphModel> _graph;

	using Views = std::map<std::shared_ptr<const client::ObjectModel>, Ganv::Module*>;
	Views _views;

	int                 _auto_position_count{0};
	std::pair<int, int> _auto_position_scroll_offsets;

	int _menu_x{0};
	int _menu_y{0};
	int _paste_count{0};

	// Track pasted objects so they can be selected when they arrive
	std::set<raul::Path> _pastees;

	Gtk::Menu*          _menu                    = nullptr;
	Gtk::Menu*          _internal_menu           = nullptr;
	PluginMenu*         _plugin_menu             = nullptr;
	Gtk::MenuItem*      _menu_add_audio_input    = nullptr;
	Gtk::MenuItem*      _menu_add_audio_output   = nullptr;
	Gtk::MenuItem*      _menu_add_control_input  = nullptr;
	Gtk::MenuItem*      _menu_add_control_output = nullptr;
	Gtk::MenuItem*      _menu_add_cv_input       = nullptr;
	Gtk::MenuItem*      _menu_add_cv_output      = nullptr;
	Gtk::MenuItem*      _menu_add_event_input    = nullptr;
	Gtk::MenuItem*      _menu_add_event_output   = nullptr;
	Gtk::MenuItem*      _menu_load_plugin        = nullptr;
	Gtk::MenuItem*      _menu_load_graph         = nullptr;
	Gtk::MenuItem*      _menu_new_graph          = nullptr;
	Gtk::MenuItem*      _menu_properties         = nullptr;
	Gtk::CheckMenuItem* _menu_edit               = nullptr;

	bool _human_names = true;
	bool _show_port_names = true;
	bool _menu_dirty = false;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_GRAPHCANVAS_HPP
