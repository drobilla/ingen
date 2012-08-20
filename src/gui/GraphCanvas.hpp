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

#ifndef INGEN_GUI_GRAPHCANVAS_HPP
#define INGEN_GUI_GRAPHCANVAS_HPP

#include <string>
#include <map>
#include <set>

#include <boost/shared_ptr.hpp>

#include "lilv/lilv.h"

#include "ganv/Canvas.hpp"
#include "ganv/Module.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/Path.hpp"

#include "NodeModule.hpp"
#include "ingen/Node.hpp"
#include "ingen/client/EdgeModel.hpp"

namespace Ingen {

namespace Client { class GraphModel; }

namespace GUI {

class NodeModule;

/** Graph canvas widget.
 *
 * \ingroup GUI
 */
class GraphCanvas : public Ganv::Canvas
{
public:
	GraphCanvas(App&                                app,
	            SharedPtr<const Client::GraphModel> graph,
	            int                                 width,
	            int                                 height);

	virtual ~GraphCanvas() {}

	App& app() { return _app; }

	void build();
	void show_human_names(bool show);
	void show_port_names(bool show);
	bool show_port_names() const { return _show_port_names; }

	void add_plugin(SharedPtr<Client::PluginModel> pm);
	void add_block(SharedPtr<const Client::BlockModel> bm);
	void remove_block(SharedPtr<const Client::BlockModel> bm);
	void add_port(SharedPtr<const Client::PortModel> pm);
	void remove_port(SharedPtr<const Client::PortModel> pm);
	void connection(SharedPtr<const Client::EdgeModel> cm);
	void disconnection(SharedPtr<const Client::EdgeModel> cm);

	void get_new_module_location(double& x, double& y);

	void clear_selection();
	void destroy_selection();
	void copy_selection();
	void paste();

	void show_menu(bool position, unsigned button, uint32_t time);

	bool on_event(GdkEvent* event);

private:
	enum ControlType { NUMBER, BUTTON };
	void generate_port_name(
		const std::string& sym_base,  std::string& sym,
		const std::string& name_base, std::string& name);

	void menu_add_port(
			const std::string& sym_base, const std::string& name_base,
			const Raul::URI& type, bool is_output);

	void menu_load_plugin();
	void menu_new_graph();
	void menu_load_graph();
	void load_plugin(WeakPtr<Client::PluginModel> plugin);

	void build_menus();

	void build_internal_menu();
	void build_classless_menu();

    void auto_menu_position(int& x, int& y, bool& push_in);

	typedef std::multimap<const std::string, const LilvPluginClass*> LV2Children;
	void build_plugin_menu();
	size_t build_plugin_class_menu(
			Gtk::Menu*               menu,
			const LilvPluginClass*   plugin_class,
			const LilvPluginClasses* classes,
			const LV2Children&       children,
			std::set<const char*>&   ancestors);

	Node::Properties get_initial_data(Resource::Graph ctx=Resource::DEFAULT);

	Ganv::Port* get_port_view(SharedPtr<Client::PortModel> port);

	void connect(Ganv::Node* src,
	             Ganv::Node* dst);

	void disconnect(Ganv::Node* src,
	                Ganv::Node* dst);

	App&                                _app;
	SharedPtr<const Client::GraphModel> _graph;

	typedef std::map<SharedPtr<const Client::ObjectModel>, Ganv::Module*> Views;
	Views _views;

	int                 _auto_position_count;
	std::pair<int, int> _auto_position_scroll_offsets;

	int _last_click_x;
	int _last_click_y;
	int _paste_count;

	// Track pasted objects so they can be selected when they arrive
	std::set<Raul::Path> _pastees;

	struct MenuRecord {
		MenuRecord(Gtk::MenuItem* i, Gtk::Menu* m) : item(i), menu(m) {}
		Gtk::MenuItem* item;
		Gtk::Menu*     menu;
	};

	typedef std::multimap<const std::string, MenuRecord> ClassMenus;

	ClassMenus _class_menus;

	Gtk::Menu*          _menu;
	Gtk::Menu*          _internal_menu;
	Gtk::Menu*          _classless_menu;
	Gtk::Menu*          _plugin_menu;
	Gtk::MenuItem*      _menu_add_audio_input;
	Gtk::MenuItem*      _menu_add_audio_output;
	Gtk::MenuItem*      _menu_add_control_input;
	Gtk::MenuItem*      _menu_add_control_output;
	Gtk::MenuItem*      _menu_add_event_input;
	Gtk::MenuItem*      _menu_add_event_output;
	Gtk::MenuItem*      _menu_load_plugin;
	Gtk::MenuItem*      _menu_load_graph;
	Gtk::MenuItem*      _menu_new_graph;
	Gtk::CheckMenuItem* _menu_edit;

	bool _human_names;
	bool _show_port_names;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_GRAPHCANVAS_HPP