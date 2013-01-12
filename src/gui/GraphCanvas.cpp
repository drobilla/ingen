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

#include <cassert>
#include <map>
#include <set>
#include <string>

#include <boost/format.hpp>
#include <gtkmm/stock.h>

#include "ganv/Canvas.hpp"
#include "ganv/Circle.hpp"
#include "ingen/Builder.hpp"
#include "ingen/ClashAvoider.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/World.hpp"
#include "ingen/client/BlockModel.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/GraphModel.hpp"
#include "ingen/client/PluginModel.hpp"
#include "ingen/serialisation/Serialiser.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

#include "App.hpp"
#include "Arc.hpp"
#include "GraphCanvas.hpp"
#include "GraphPortModule.hpp"
#include "GraphWindow.hpp"
#include "LoadPluginWindow.hpp"
#include "NewSubgraphWindow.hpp"
#include "NodeModule.hpp"
#include "Port.hpp"
#include "SubgraphModule.hpp"
#include "ThreadedLoader.hpp"
#include "WidgetFactory.hpp"
#include "WindowFactory.hpp"

using namespace std;

namespace Ingen {

using namespace Client;

namespace GUI {

GraphCanvas::GraphCanvas(App&                   app,
                         SPtr<const GraphModel> graph,
                         int                    width,
                         int                    height)
	: Canvas(width, height)
	, _app(app)
	, _graph(graph)
	, _auto_position_count(0)
	, _last_click_x(0)
	, _last_click_y(0)
	, _paste_count(0)
	, _menu(NULL)
	, _internal_menu(NULL)
	, _classless_menu(NULL)
	, _plugin_menu(NULL)
	, _human_names(true)
	, _show_port_names(true)
{
	Glib::RefPtr<Gtk::Builder> xml = WidgetFactory::create("canvas_menu");
	xml->get_widget("canvas_menu", _menu);

	xml->get_widget("canvas_menu_add_audio_input", _menu_add_audio_input);
	xml->get_widget("canvas_menu_add_audio_output", _menu_add_audio_output);
	xml->get_widget("canvas_menu_add_cv_input", _menu_add_cv_input);
	xml->get_widget("canvas_menu_add_cv_output", _menu_add_cv_output);
	xml->get_widget("canvas_menu_add_control_input", _menu_add_control_input);
	xml->get_widget("canvas_menu_add_control_output", _menu_add_control_output);
	xml->get_widget("canvas_menu_add_event_input", _menu_add_event_input);
	xml->get_widget("canvas_menu_add_event_output", _menu_add_event_output);
	xml->get_widget("canvas_menu_load_plugin", _menu_load_plugin);
	xml->get_widget("canvas_menu_load_graph", _menu_load_graph);
	xml->get_widget("canvas_menu_new_graph", _menu_new_graph);
	xml->get_widget("canvas_menu_edit", _menu_edit);
	xml->get_widget("canvas_menu_properties", _menu_properties);

	const URIs& uris = _app.uris();

	// Add port menu items
	_menu_add_audio_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &GraphCanvas::menu_add_port),
		           "audio_in", "Audio In", uris.lv2_AudioPort, false));
	_menu_add_audio_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &GraphCanvas::menu_add_port),
		           "audio_out", "Audio Out", uris.lv2_AudioPort, true));
	_menu_add_cv_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &GraphCanvas::menu_add_port),
		           "cv_in", "CV In", uris.lv2_CVPort, false));
	_menu_add_cv_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &GraphCanvas::menu_add_port),
		           "cv_out", "CV Out", uris.lv2_CVPort, true));
	_menu_add_control_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &GraphCanvas::menu_add_port),
		           "control_in", "Control In", uris.lv2_ControlPort, false));
	_menu_add_control_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &GraphCanvas::menu_add_port),
		           "control_out", "Control Out", uris.lv2_ControlPort, true));
	_menu_add_event_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &GraphCanvas::menu_add_port),
		           "event_in", "Event In", uris.atom_AtomPort, false));
	_menu_add_event_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &GraphCanvas::menu_add_port),
		           "event_out", "Event Out", uris.atom_AtomPort, true));

	signal_event.connect(
		sigc::mem_fun(this, &GraphCanvas::on_event));
	signal_connect.connect(
		sigc::mem_fun(this, &GraphCanvas::connect));
	signal_disconnect.connect(
		sigc::mem_fun(this, &GraphCanvas::disconnect));

	// Connect to model signals to track state
	_graph->signal_new_block().connect(
		sigc::mem_fun(this, &GraphCanvas::add_block));
	_graph->signal_removed_block().connect(
		sigc::mem_fun(this, &GraphCanvas::remove_block));
	_graph->signal_new_port().connect(
		sigc::mem_fun(this, &GraphCanvas::add_port));
	_graph->signal_removed_port().connect(
		sigc::mem_fun(this, &GraphCanvas::remove_port));
	_graph->signal_new_arc().connect(
		sigc::mem_fun(this, &GraphCanvas::connection));
	_graph->signal_removed_arc().connect(
		sigc::mem_fun(this, &GraphCanvas::disconnection));

	_app.store()->signal_new_plugin().connect(
		sigc::mem_fun(this, &GraphCanvas::add_plugin));

	// Connect widget signals to do things
	_menu_load_plugin->signal_activate().connect(
		sigc::mem_fun(this, &GraphCanvas::menu_load_plugin));
	_menu_load_graph->signal_activate().connect(
		sigc::mem_fun(this, &GraphCanvas::menu_load_graph));
	_menu_new_graph->signal_activate().connect(
		sigc::mem_fun(this, &GraphCanvas::menu_new_graph));
	_menu_properties->signal_activate().connect(
		sigc::mem_fun(this, &GraphCanvas::menu_properties));

	show_human_names(app.world()->conf().option("human-names").get_bool());
	show_port_names(app.world()->conf().option("port-labels").get_bool());
}

void
GraphCanvas::show_menu(bool position, unsigned button, uint32_t time)
{
	_app.request_plugins_if_necessary();

	if (!_internal_menu)
		build_menus();

	if (position)
		_menu->popup(sigc::mem_fun(this, &GraphCanvas::auto_menu_position), button, time);
	else
		_menu->popup(button, time);
}

void
GraphCanvas::build_menus()
{
	// Build (or clear existing) internal plugin menu
	if (_internal_menu) {
		_internal_menu->items().clear();
	} else {
		_menu->items().push_back(
			Gtk::Menu_Helpers::ImageMenuElem(
				"In_ternal",
				*(manage(new Gtk::Image(Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_MENU)))));
		Gtk::MenuItem* internal_menu_item = &(_menu->items().back());
		_internal_menu = Gtk::manage(new Gtk::Menu());
		internal_menu_item->set_submenu(*_internal_menu);
		_menu->reorder_child(*internal_menu_item, 4);
	}

	// Build skeleton LV2 plugin class heirarchy for 'Plugin' menu
	if (!_plugin_menu)
		build_plugin_menu();

	// Build (or clear existing) uncategorized (classless, heh) plugins menu
	if (_classless_menu) {
		_classless_menu->items().clear();
	} else {
		_plugin_menu->items().push_back(Gtk::Menu_Helpers::MenuElem("_Uncategorized"));
		Gtk::MenuItem* classless_menu_item = &(_plugin_menu->items().back());
		_classless_menu = Gtk::manage(new Gtk::Menu());
		classless_menu_item->set_submenu(*_classless_menu);
		_classless_menu->hide();
	}

	// Add known plugins to menu heirarchy
	SPtr<const ClientStore::Plugins> plugins = _app.store()->plugins();
	for (const auto& p : *plugins.get())
		add_plugin(p.second);
}

/** Recursively build the plugin class menu heirarchy rooted at
 * @a plugin class into @a menu
 */
size_t
GraphCanvas::build_plugin_class_menu(
	Gtk::Menu*               menu,
	const LilvPluginClass*   plugin_class,
	const LilvPluginClasses* classes,
	const LV2Children&       children,
	std::set<const char*>&   ancestors)
{
	size_t          num_items     = 0;
	const LilvNode* class_uri     = lilv_plugin_class_get_uri(plugin_class);
	const char*     class_uri_str = lilv_node_as_string(class_uri);

	const std::pair<LV2Children::const_iterator, LV2Children::const_iterator> kids
		= children.equal_range(class_uri_str);

	if (kids.first == children.end())
		return 0;

	// Add submenus
	ancestors.insert(class_uri_str);
	for (LV2Children::const_iterator i = kids.first; i != kids.second; ++i) {
		const LilvPluginClass* c = i->second;
		const char* sub_label_str = lilv_node_as_string(lilv_plugin_class_get_label(c));
		const char* sub_uri_str   = lilv_node_as_string(lilv_plugin_class_get_uri(c));
		if (ancestors.find(sub_uri_str) != ancestors.end()) {
			_app.log().warn(Raul::fmt("Infinite LV2 class recursion: %1% <: %2%\n")
			                % class_uri_str % sub_uri_str);
			return 0;
		}

		Gtk::Menu_Helpers::MenuElem menu_elem = Gtk::Menu_Helpers::MenuElem(
			std::string("_") + sub_label_str);
		menu->items().push_back(menu_elem);
		Gtk::MenuItem* menu_item = &(menu->items().back());

		Gtk::Menu* submenu = Gtk::manage(new Gtk::Menu());
		menu_item->set_submenu(*submenu);

		size_t num_child_items = build_plugin_class_menu(
			submenu, c, classes, children, ancestors);

		_class_menus.insert(make_pair(sub_uri_str, MenuRecord(menu_item, submenu)));
		if (num_child_items == 0)
			menu_item->hide();

		++num_items;
	}
	ancestors.erase(class_uri_str);

	return num_items;
}

void
GraphCanvas::build_plugin_menu()
{
	if (_plugin_menu) {
		_plugin_menu->items().clear();
	} else {
		_menu->items().push_back(
			Gtk::Menu_Helpers::ImageMenuElem(
				"_Plugin",
				*(manage(new Gtk::Image(Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_MENU)))));
		Gtk::MenuItem* plugin_menu_item = &(_menu->items().back());
		_plugin_menu = Gtk::manage(new Gtk::Menu());
		plugin_menu_item->set_submenu(*_plugin_menu);
		_menu->reorder_child(*plugin_menu_item, 5);
	}

	const LilvWorld*         world      = PluginModel::lilv_world();
	const LilvPluginClass*   lv2_plugin = lilv_world_get_plugin_class(world);
	const LilvPluginClasses* classes    = lilv_world_get_plugin_classes(world);

	LV2Children children;
	LILV_FOREACH(plugin_classes, i, classes) {
		const LilvPluginClass* c = lilv_plugin_classes_get(classes, i);
		const LilvNode*        p = lilv_plugin_class_get_parent_uri(c);
		if (!p)
			p = lilv_plugin_class_get_uri(lv2_plugin);
		children.insert(make_pair(lilv_node_as_string(p), c));
	}
	std::set<const char*> ancestors;
	build_plugin_class_menu(_plugin_menu, lv2_plugin, classes, children, ancestors);
}

void
GraphCanvas::build()
{
	const Store::const_range kids = _app.store()->children_range(_graph);

	// Create modules for blocks
	for (Store::const_iterator i = kids.first; i != kids.second; ++i) {
		SPtr<BlockModel> block = dynamic_ptr_cast<BlockModel>(i->second);
		if (block && block->parent() == _graph)
			add_block(block);
	}

	// Create pseudo modules for ports (ports on this canvas, not on our module)
	for (const auto& p : _graph->ports()) {
		add_port(p);
	}

	// Create arcs
	for (const auto& a : _graph->arcs()) {
		connection(dynamic_ptr_cast<ArcModel>(a.second));
	}
}

static void
show_module_human_names(GanvNode* node, void* data)
{
	bool b = *(bool*)data;
	if (GANV_IS_MODULE(node)) {
		Ganv::Module* module = Glib::wrap(GANV_MODULE(node));
		NodeModule* nmod = dynamic_cast<NodeModule*>(module);
		if (nmod)
			nmod->show_human_names(b);

		GraphPortModule* pmod = dynamic_cast<GraphPortModule*>(module);
		if (pmod)
			pmod->show_human_names(b);
	}
}

void
GraphCanvas::show_human_names(bool b)
{
	_human_names = b;
	for_each_node(show_module_human_names, &b);
}

void
GraphCanvas::show_port_names(bool b)
{
	ganv_canvas_set_direction(gobj(), b ? GANV_DIRECTION_RIGHT : GANV_DIRECTION_DOWN);
}

void
GraphCanvas::add_plugin(SPtr<PluginModel> p)
{
	typedef ClassMenus::iterator iterator;
	if (_internal_menu && p->type() == Plugin::Internal) {
		_internal_menu->items().push_back(
			Gtk::Menu_Helpers::MenuElem(
				std::string("_") + p->human_name(),
				sigc::bind(sigc::mem_fun(this, &GraphCanvas::load_plugin), p)));
	} else if (_plugin_menu && p->type() == Plugin::LV2 && p->lilv_plugin()) {
		if (lilv_plugin_is_replaced(p->lilv_plugin())) {
			//info << (boost::format("[Menu] LV2 plugin <%s> hidden") % p->uri()) << endl;
			return;
		}

		const LilvPluginClass* pc            = lilv_plugin_get_class(p->lilv_plugin());
		const LilvNode*        class_uri     = lilv_plugin_class_get_uri(pc);
		const char*            class_uri_str = lilv_node_as_string(class_uri);

		Glib::RefPtr<Gdk::Pixbuf> icon = _app.icon_from_path(
			PluginModel::get_lv2_icon_path(p->lilv_plugin()), 16);

		pair<iterator, iterator> range = _class_menus.equal_range(class_uri_str);
		if (range.first == _class_menus.end() || range.first == range.second
		    || range.first->second.menu == _plugin_menu) {
			_classless_menu->items().push_back(
				Gtk::Menu_Helpers::MenuElem(
					std::string("_") + p->human_name(),
					sigc::bind(sigc::mem_fun(this, &GraphCanvas::load_plugin), p)));
			if (!_classless_menu->is_visible())
				_classless_menu->show();
		} else {
			// For each menu that represents plugin's class (possibly several)
			for (iterator i = range.first; i != range.second ; ++i) {
				Gtk::Menu* menu = i->second.menu;
				if (icon) {
					Gtk::Image* image = new Gtk::Image(icon);
					menu->items().push_back(
						Gtk::Menu_Helpers::ImageMenuElem(
							std::string("_") + p->human_name(), *image,
							sigc::bind(sigc::mem_fun(this, &GraphCanvas::load_plugin), p)));
				} else {
					menu->items().push_back(
						Gtk::Menu_Helpers::MenuElem(
							std::string("_") + p->human_name(),
							sigc::bind(sigc::mem_fun(this, &GraphCanvas::load_plugin), p)));
				}
				if (!i->second.item->is_visible())
					i->second.item->show();
			}
		}
	}
}

void
GraphCanvas::add_block(SPtr<const BlockModel> bm)
{
	SPtr<const GraphModel> pm = dynamic_ptr_cast<const GraphModel>(bm);
	NodeModule*            module;
	if (pm) {
		module = SubgraphModule::create(*this, pm, _human_names);
	} else {
		module = NodeModule::create(*this, bm, _human_names);
		//const PluginModel* plugm = dynamic_cast<const PluginModel*>(nm->plugin());
		//if (plugm && !plugm->icon_path().empty())
		//	module->set_icon(_app.icon_from_path(plugm->icon_path(), 100).operator->());
	}

	module->show();
	_views.insert(std::make_pair(bm, module));
	if (_pastees.find(bm->path()) != _pastees.end()) {
		module->set_selected(true);
	}
}

void
GraphCanvas::remove_block(SPtr<const BlockModel> bm)
{
	Views::iterator i = _views.find(bm);

	if (i != _views.end()) {
		const guint n_ports = i->second->num_ports();
		for (gint p = n_ports - 1; p >= 0; --p) {
			delete i->second->get_port(p);
		}
		delete i->second;
		_views.erase(i);
	}
}

void
GraphCanvas::add_port(SPtr<const PortModel> pm)
{
	GraphPortModule* view = GraphPortModule::create(*this, pm, _human_names);
	_views.insert(std::make_pair(pm, view));
	view->show();
}

void
GraphCanvas::remove_port(SPtr<const PortModel> pm)
{
	Views::iterator i = _views.find(pm);

	// Port on this graph
	if (i != _views.end()) {
		delete i->second;
		_views.erase(i);

	} else {
		NodeModule* module = dynamic_cast<NodeModule*>(_views[pm->parent()]);
		module->delete_port_view(pm);
	}

	assert(_views.find(pm) == _views.end());
}

Ganv::Port*
GraphCanvas::get_port_view(SPtr<PortModel> port)
{
	Ganv::Module* module = _views[port];

	// Port on this graph
	if (module) {
		GraphPortModule* ppm = dynamic_cast<GraphPortModule*>(module);
		return ppm
			? *ppm->begin()
			: dynamic_cast<Ganv::Port*>(module);
	} else {
		module = dynamic_cast<NodeModule*>(_views[port->parent()]);
		if (module) {
			for (const auto& p : *module) {
				GUI::Port* pv = dynamic_cast<GUI::Port*>(p);
				if (pv && pv->model() == port)
					return pv;
			}
		}
	}

	return NULL;
}

void
GraphCanvas::connection(SPtr<const ArcModel> arc)
{
	Ganv::Port* const tail = get_port_view(arc->tail());
	Ganv::Port* const head = get_port_view(arc->head());

	if (tail && head) {
		new GUI::Arc(*this, arc, tail, head, tail->get_fill_color());
	} else {
		_app.log().error(Raul::fmt("Unable to find ports to connect %1% => %2%\n")
		                 % arc->tail_path() % arc->head_path());
	}
}

void
GraphCanvas::disconnection(SPtr<const ArcModel> arc)
{
	Ganv::Port* const src = get_port_view(arc->tail());
	Ganv::Port* const dst = get_port_view(arc->head());

	if (src && dst) {
		remove_edge(src, dst);
	} else {
		_app.log().error(Raul::fmt("Unable to find ports to disconnect %1% => %2%\n")
		                 % arc->tail_path() % arc->head_path());
	}
}

void
GraphCanvas::connect(Ganv::Node* tail,
                     Ganv::Node* head)
{
	const Ingen::GUI::Port* const src
		= dynamic_cast<Ingen::GUI::Port*>(tail);

	const Ingen::GUI::Port* const dst
		= dynamic_cast<Ingen::GUI::Port*>(head);

	if (!src || !dst)
		return;

	_app.interface()->connect(src->model()->path(), dst->model()->path());
}

void
GraphCanvas::disconnect(Ganv::Node* tail,
                        Ganv::Node* head)
{
	const Ingen::GUI::Port* const t = dynamic_cast<Ingen::GUI::Port*>(tail);
	const Ingen::GUI::Port* const h = dynamic_cast<Ingen::GUI::Port*>(head);

	_app.interface()->disconnect(t->model()->path(), h->model()->path());
}

void
GraphCanvas::auto_menu_position(int& x, int& y, bool& push_in)
{
	std::pair<int, int> scroll_offsets;
	get_scroll_offsets(scroll_offsets.first, scroll_offsets.second);

	if (_auto_position_count > 0 && scroll_offsets != _auto_position_scroll_offsets)
		_auto_position_count = 0; // scrolling happened since last time, reset

	const int cascade = (_auto_position_count > 0) ? (_auto_position_count * 32) : 0;

	x = 64 + cascade;
	y = 64 + cascade;
	push_in = true;

	_last_click_x = scroll_offsets.first  + x;
	_last_click_y = scroll_offsets.second + y;

	++_auto_position_count;
	_auto_position_scroll_offsets = scroll_offsets;
}

bool
GraphCanvas::on_event(GdkEvent* event)
{
	assert(event);

	bool ret = false;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 3) {
			_auto_position_count = 0;
			_last_click_x = (int)event->button.x;
			_last_click_y = (int)event->button.y;
			show_menu(false, event->button.button, event->button.time);
			ret = true;
		}
		break;

	case GDK_KEY_PRESS:
		switch (event->key.keyval) {
		case GDK_Delete:
			destroy_selection();
			ret = true;
			break;
		case GDK_Home:
			scroll_to(0, 0);
			break;
		case GDK_space:
		case GDK_Menu:
			show_menu(true, 3, event->key.time);
		default: break;
		}

	default: break;
	}

	return ret;
}

void
GraphCanvas::clear_selection()
{
	GraphWindow* win = _app.window_factory()->graph_window(_graph);
	if (win) {
		win->set_documentation("", false);
	}

	Ganv::Canvas::clear_selection();
}

static void
destroy_node(GanvNode* node, void* data)
{
	if (!GANV_IS_MODULE(node)) {
		return;
	}

	App*          app         = (App*)data;
	Ganv::Module* module      = Glib::wrap(GANV_MODULE(node));
	NodeModule*   node_module = dynamic_cast<NodeModule*>(module);

	if (node_module) {
		app->interface()->del(node_module->block()->uri());
	} else {
		GraphPortModule* port_module = dynamic_cast<GraphPortModule*>(module);
		if (port_module) {
			app->interface()->del(port_module->port()->uri());
		}
	}
}

static void
destroy_arc(GanvEdge* arc, void* data)
{
	App*        app   = (App*)data;
	Ganv::Edge* arcmm = Glib::wrap(arc);

	Port* tail = dynamic_cast<Port*>(arcmm->get_tail());
	Port* head = dynamic_cast<Port*>(arcmm->get_head());
	app->interface()->disconnect(tail->model()->path(), head->model()->path());
}

void
GraphCanvas::destroy_selection()
{
	for_each_selected_node(destroy_node, &_app);
	for_each_selected_edge(destroy_arc, &_app);
}

static void
serialise_node(GanvNode* node, void* data)
{
	Serialisation::Serialiser* serialiser = (Serialisation::Serialiser*)data;
	if (!GANV_IS_MODULE(node)) {
		return;
	}

	Ganv::Module* module      = Glib::wrap(GANV_MODULE(node));
	NodeModule*   node_module = dynamic_cast<NodeModule*>(module);

	if (node_module) {
		serialiser->serialise(node_module->block());
	} else {
		GraphPortModule* port_module = dynamic_cast<GraphPortModule*>(module);
		if (port_module) {
			serialiser->serialise(port_module->port());
		}
	}
}

static void
serialise_arc(GanvEdge* arc, void* data)
{
	Serialisation::Serialiser* serialiser = (Serialisation::Serialiser*)data;
	if (!GANV_IS_EDGE(arc)) {
		return;
	}

	GUI::Arc* garc = dynamic_cast<GUI::Arc*>(Glib::wrap(GANV_EDGE(arc)));
	if (garc) {
		serialiser->serialise_arc(Sord::Node(), garc->model());
	}
}

void
GraphCanvas::copy_selection()
{
	static const char* base_uri = "http://drobilla.net/ns/ingen/selection/";
	Serialisation::Serialiser serialiser(*_app.world());
	serialiser.start_to_string(_graph->path(), base_uri);

	for_each_selected_node(serialise_node, &serialiser);
	for_each_selected_edge(serialise_arc, &serialiser);

	const std::string result = serialiser.finish();
	_paste_count = 0;

	Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
	clipboard->set_text(result);
}

void
GraphCanvas::paste()
{
	Glib::ustring str = Gtk::Clipboard::get()->wait_for_text();
	SPtr<Serialisation::Parser> parser = _app.loader()->parser();
	if (!parser) {
		_app.log().error("Unable to load parser, paste unavailable\n");
		return;
	}

	clear_selection();
	_pastees.clear();
	++_paste_count;

	const URIs& uris = _app.uris();

	Builder builder(_app.world()->uris(), *_app.interface());
	ClientStore clipboard(_app.world()->uris(), _app.log());
	clipboard.set_plugins(_app.store()->plugins());

	// mkdir -p
	string to_create = _graph->path().substr(1);
	string created = "/";
	Resource::Properties props;
	props.insert(make_pair(uris.rdf_type,
	                       Resource::Property(uris.ingen_Graph)));
	props.insert(make_pair(uris.ingen_polyphony,
	                       _app.forge().make(int32_t(_graph->internal_poly()))));
	clipboard.put(Node::root_uri(), props);
	size_t first_slash;
	while (to_create != "/" && !to_create.empty()
	       && (first_slash = to_create.find("/")) != string::npos) {
		created += to_create.substr(0, first_slash);
		assert(Raul::Path::is_valid(created));
		clipboard.put(Node::path_to_uri(Raul::Path(created)), props);
		to_create = to_create.substr(first_slash + 1);
	}

	if (!_graph->path().is_root())
		clipboard.put(_graph->uri(), props);

	boost::optional<Raul::Path>   parent;
	boost::optional<Raul::Symbol> symbol;

	if (!_graph->path().is_root()) {
		parent = _graph->path();
	}

	ClashAvoider avoider(*_app.store().get(), clipboard, &clipboard);
	static const char* base_uri = "http://drobilla.net/ns/ingen/selection/";
	parser->parse_string(_app.world(), &avoider, str, base_uri,
	                     parent, symbol);

	for (const auto& c : clipboard) {
		if (_graph->path().is_root() && c.first.is_root())
			continue;

		Node::Properties& props = c.second->properties();

		Node::Properties::iterator x = props.find(uris.ingen_canvasX);
		if (x != c.second->properties().end())
			x->second = _app.forge().make(
				x->second.get_float() + (20.0f * _paste_count));

		Node::Properties::iterator y = props.find(uris.ingen_canvasY);
		if (y != c.second->properties().end())
			y->second = _app.forge().make(
				y->second.get_float() + (20.0f * _paste_count));

		builder.build(c.second);
		_pastees.insert(c.first);
	}

	builder.connect(
		dynamic_ptr_cast<const GraphModel>(clipboard.object(_graph->path())));
}

void
GraphCanvas::generate_port_name(
	const string& sym_base,  string& symbol,
	const string& name_base, string& name)
{
	symbol = sym_base;
	name   = name_base;

	char num_buf[5];
	uint32_t i = 1;
	for ( ; i < 9999; ++i) {
		snprintf(num_buf, sizeof(num_buf), "%u", i);
		symbol = sym_base + "_";
		symbol += num_buf;
		if (!_graph->get_port(Raul::Symbol::symbolify(symbol)))
			break;
	}

	assert(Raul::Path::is_valid(string("/") + symbol));

	name.append(" ").append(num_buf);
}

void
GraphCanvas::menu_add_port(const string& sym_base, const string& name_base,
                           const Raul::URI& type, bool is_output)
{
	string sym, name;
	generate_port_name(sym_base, sym, name_base, name);
	const Raul::Path& path = _graph->path().child(Raul::Symbol(sym));

	const URIs& uris = _app.uris();

	Resource::Properties props = get_initial_data();
	props.insert(make_pair(uris.rdf_type,
	                       _app.forge().alloc_uri(type)));
	if (type == uris.atom_AtomPort) {
		props.insert(make_pair(uris.atom_bufferType,
		                       Resource::Property(uris.atom_Sequence)));
	}
	props.insert(make_pair(uris.rdf_type,
	                       Resource::Property(is_output
	                                          ? uris.lv2_OutputPort
	                                          : uris.lv2_InputPort)));
	props.insert(make_pair(uris.lv2_index,
	                       _app.forge().make(int32_t(_graph->num_ports()))));
	props.insert(make_pair(uris.lv2_name,
	                       _app.forge().alloc(name.c_str())));
	_app.interface()->put(Node::path_to_uri(path), props);
}

void
GraphCanvas::load_plugin(WPtr<PluginModel> weak_plugin)
{
	SPtr<PluginModel> plugin = weak_plugin.lock();
	if (!plugin)
		return;

	Raul::Symbol symbol = plugin->default_block_symbol();
	unsigned offset = _app.store()->child_name_offset(_graph->path(), symbol);
	if (offset != 0) {
		std::stringstream ss;
		ss << symbol << "_" << offset;
		symbol = Raul::Symbol(ss.str());
	}

	const URIs&      uris = _app.uris();
	const Raul::Path path = _graph->path().child(symbol);

	// FIXME: polyphony?
	Node::Properties props = get_initial_data();
	props.insert(make_pair(uris.rdf_type,
	                       Resource::Property(uris.ingen_Block)));
	props.insert(make_pair(uris.ingen_prototype,
	                       uris.forge.alloc_uri(plugin->uri())));
	_app.interface()->put(Node::path_to_uri(path), props);
}

/** Try to guess a suitable location for a new module.
 */
void
GraphCanvas::get_new_module_location(double& x, double& y)
{
	int scroll_x;
	int scroll_y;
	get_scroll_offsets(scroll_x, scroll_y);
	x = scroll_x + 20;
	y = scroll_y + 20;
}

Node::Properties
GraphCanvas::get_initial_data(Resource::Graph ctx)
{
	Node::Properties result;
	const URIs& uris = _app.uris();
	result.insert(
		make_pair(uris.ingen_canvasX,
		          Resource::Property(_app.forge().make((float)_last_click_x),
		                             ctx)));
	result.insert(
		make_pair(uris.ingen_canvasY,
		          Resource::Property(_app.forge().make((float)_last_click_y),
		                             ctx)));
	return result;
}

void
GraphCanvas::menu_load_plugin()
{
	_app.window_factory()->present_load_plugin(
		_graph, get_initial_data());
}

void
GraphCanvas::menu_load_graph()
{
	_app.window_factory()->present_load_subgraph(
		_graph, get_initial_data(Resource::Graph::EXTERNAL));
}

void
GraphCanvas::menu_new_graph()
{
	_app.window_factory()->present_new_subgraph(
		_graph, get_initial_data(Resource::Graph::EXTERNAL));
}

void
GraphCanvas::menu_properties()
{
	_app.window_factory()->present_properties(_graph);
}

} // namespace GUI
} // namespace Ingen
