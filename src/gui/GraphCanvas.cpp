/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "App.hpp"
#include "Arc.hpp"
#include "GraphCanvas.hpp"
#include "GraphPortModule.hpp"
#include "GraphWindow.hpp"
#include "LoadPluginWindow.hpp"
#include "NewSubgraphWindow.hpp"
#include "NodeModule.hpp"
#include "PluginMenu.hpp"
#include "Port.hpp"
#include "SubgraphModule.hpp"
#include "ThreadedLoader.hpp"
#include "WidgetFactory.hpp"
#include "WindowFactory.hpp"

#include "ganv/Canvas.hpp"
#include "ganv/Circle.hpp"
#include "ingen/ClashAvoider.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/Serialiser.hpp"
#include "ingen/World.hpp"
#include "ingen/client/BlockModel.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/GraphModel.hpp"
#include "ingen/client/PluginModel.hpp"
#include "ingen/ingen.h"
#include "lv2/atom/atom.h"

#include <boost/optional/optional.hpp>
#include <gtkmm/stock.h>

#include <algorithm>
#include <cassert>
#include <map>
#include <set>
#include <string>

using std::string;

namespace ingen {

using namespace client;

namespace gui {

static int
port_order(const GanvPort* a, const GanvPort* b, void* data)
{
	const Port* pa = dynamic_cast<const Port*>(Glib::wrap(a));
	const Port* pb = dynamic_cast<const Port*>(Glib::wrap(b));
	if (pa && pb) {
		return (static_cast<int>(pa->model()->index()) -
		        static_cast<int>(pb->model()->index()));
	}
	return 0;
}

GraphCanvas::GraphCanvas(App&                   app,
                         SPtr<const GraphModel> graph,
                         int                    width,
                         int                    height)
	: Canvas(width, height)
	, _app(app)
	, _graph(std::move(graph))
	, _auto_position_count(0)
	, _menu_x(0)
	, _menu_y(0)
	, _paste_count(0)
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
	_app.store()->signal_plugin_deleted().connect(
		sigc::mem_fun(this, &GraphCanvas::remove_plugin));

	// Connect widget signals to do things
	_menu_load_plugin->signal_activate().connect(
		sigc::mem_fun(this, &GraphCanvas::menu_load_plugin));
	_menu_load_graph->signal_activate().connect(
		sigc::mem_fun(this, &GraphCanvas::menu_load_graph));
	_menu_new_graph->signal_activate().connect(
		sigc::mem_fun(this, &GraphCanvas::menu_new_graph));
	_menu_properties->signal_activate().connect(
		sigc::mem_fun(this, &GraphCanvas::menu_properties));

	show_human_names(app.world().conf().option("human-names").get<int32_t>());
	show_port_names(app.world().conf().option("port-labels").get<int32_t>());
	set_port_order(port_order, nullptr);
}

void
GraphCanvas::show_menu(bool position, unsigned button, uint32_t time)
{
	_app.request_plugins_if_necessary();

	if (!_internal_menu || _menu_dirty) {
		build_menus();
	}

	if (position) {
		_menu->popup(sigc::mem_fun(this, &GraphCanvas::auto_menu_position), button, time);
	} else {
		_menu->popup(button, time);
	}
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
	if (_plugin_menu) {
		_plugin_menu->clear();
	} else {
		_plugin_menu = Gtk::manage(new PluginMenu(_app.world()));
		_menu->items().push_back(
			Gtk::Menu_Helpers::ImageMenuElem(
				"_Plugin",
				*(manage(new Gtk::Image(Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_MENU)))));
		Gtk::MenuItem* plugin_menu_item = &(_menu->items().back());
		plugin_menu_item->set_submenu(*_plugin_menu);
		_menu->reorder_child(*plugin_menu_item, 5);
		_plugin_menu->signal_load_plugin.connect(
			sigc::mem_fun(this, &GraphCanvas::load_plugin));
	}

	// Add known plugins to menu heirarchy
	SPtr<const ClientStore::Plugins> plugins = _app.store()->plugins();
	for (const auto& p : *plugins.get()) {
		add_plugin(p.second);
	}

	_menu_dirty = false;
}

void
GraphCanvas::build()
{
	const Store::const_range kids = _app.store()->children_range(_graph);

	// Create modules for blocks
	for (Store::const_iterator i = kids.first; i != kids.second; ++i) {
		SPtr<BlockModel> block = dynamic_ptr_cast<BlockModel>(i->second);
		if (block && block->parent() == _graph) {
			add_block(block);
		}
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
	bool b = *static_cast<bool*>(data);
	if (GANV_IS_MODULE(node)) {
		Ganv::Module* module = Glib::wrap(GANV_MODULE(node));
		auto* nmod = dynamic_cast<NodeModule*>(module);
		if (nmod) {
			nmod->show_human_names(b);
		}

		auto* pmod = dynamic_cast<GraphPortModule*>(module);
		if (pmod) {
			pmod->show_human_names(b);
		}
	}
}

void
GraphCanvas::show_human_names(bool b)
{
	_human_names = b;
	_app.world().conf().set("human-names", _app.forge().make(b));

	for_each_node(show_module_human_names, &b);
}

static void
ensure_port_labels(GanvNode* node, void* data)
{
	if (GANV_IS_MODULE(node)) {
		Ganv::Module* module = Glib::wrap(GANV_MODULE(node));
		for (Ganv::Port* p : *module) {
			auto* port = dynamic_cast<ingen::gui::Port*>(p);
			if (port) {
				port->ensure_label();
			}
		}
	}
}

void
GraphCanvas::show_port_names(bool b)
{
	ganv_canvas_set_direction(gobj(), b ? GANV_DIRECTION_RIGHT : GANV_DIRECTION_DOWN);
	for_each_node(ensure_port_labels, &b);
}

void
GraphCanvas::add_plugin(const SPtr<PluginModel>& p)
{
	if (_internal_menu && _app.uris().ingen_Internal == p->type()) {
		_internal_menu->items().push_back(
			Gtk::Menu_Helpers::MenuElem(
				std::string("_") + p->human_name(),
				sigc::bind(sigc::mem_fun(this, &GraphCanvas::load_plugin), p)));
	} else if (_plugin_menu) {
		_plugin_menu->add_plugin(p);
	}
}

void
GraphCanvas::remove_plugin(const URI& uri)
{
	// Flag menus as dirty so they will be rebuilt when needed next
	_menu_dirty = true;
}

void
GraphCanvas::add_block(const SPtr<const BlockModel>& bm)
{
	SPtr<const GraphModel> pm     = dynamic_ptr_cast<const GraphModel>(bm);
	NodeModule*            module = nullptr;
	if (pm) {
		module = SubgraphModule::create(*this, pm, _human_names);
	} else {
		module = NodeModule::create(*this, bm, _human_names);
	}

	module->show();
	_views.emplace(bm, module);
	if (_pastees.find(bm->path()) != _pastees.end()) {
		module->set_selected(true);
	}
}

void
GraphCanvas::remove_block(const SPtr<const BlockModel>& bm)
{
	auto i = _views.find(bm);

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
GraphCanvas::add_port(const SPtr<const PortModel>& pm)
{
	GraphPortModule* view = GraphPortModule::create(*this, pm);
	_views.emplace(pm, view);
	view->show();
}

void
GraphCanvas::remove_port(const SPtr<const PortModel>& pm)
{
	auto i = _views.find(pm);

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
GraphCanvas::get_port_view(const SPtr<PortModel>& port)
{
	Ganv::Module* module = _views[port];

	// Port on this graph
	if (module) {
		auto* ppm = dynamic_cast<GraphPortModule*>(module);
		return ppm
			? *ppm->begin()
			: dynamic_cast<Ganv::Port*>(module);
	} else {
		module = dynamic_cast<NodeModule*>(_views[port->parent()]);
		if (module) {
			for (auto* p : *module) {
				auto* pv = dynamic_cast<gui::Port*>(p);
				if (pv && pv->model() == port) {
					return pv;
				}
			}
		}
	}

	return nullptr;
}

/** Called when a connection is added to the model. */
void
GraphCanvas::connection(const SPtr<const ArcModel>& arc)
{
	Ganv::Port* const tail = get_port_view(arc->tail());
	Ganv::Port* const head = get_port_view(arc->head());

	if (tail && head) {
		new gui::Arc(*this, arc, tail, head);
	} else {
		_app.log().error("Unable to find ports to connect %1% => %2%\n",
		                 arc->tail_path(), arc->head_path());
	}
}

/** Called when a connection is removed from the model. */
void
GraphCanvas::disconnection(const SPtr<const ArcModel>& arc)
{
	Ganv::Port* const tail = get_port_view(arc->tail());
	Ganv::Port* const head = get_port_view(arc->head());

	if (tail && head) {
		remove_edge_between(tail, head);
		if (arc->head()->is_a(_app.uris().lv2_AudioPort)) {
			auto* const h = dynamic_cast<gui::Port*>(head);
			if (h) {
				h->activity(_app.forge().make(0.0f));  // Reset peaks
			}
		}
	} else {
		_app.log().error("Unable to find ports to disconnect %1% => %2%\n",
		                 arc->tail_path(), arc->head_path());
	}
}

/** Called when the user connects ports on the canvas. */
void
GraphCanvas::connect(Ganv::Node* tail, Ganv::Node* head)
{
	const gui::Port* const t = dynamic_cast<gui::Port*>(tail);
	const gui::Port* const h = dynamic_cast<gui::Port*>(head);

	if (t && h) {
		_app.interface()->connect(t->model()->path(), h->model()->path());
	}
}

/** Called when the user disconnects ports on the canvas. */
void
GraphCanvas::disconnect(Ganv::Node* tail, Ganv::Node* head)
{
	const gui::Port* const t = dynamic_cast<gui::Port*>(tail);
	const gui::Port* const h = dynamic_cast<gui::Port*>(head);

	if (t && h) {
		_app.interface()->disconnect(t->model()->path(), h->model()->path());
	}
}

void
GraphCanvas::auto_menu_position(int& x, int& y, bool& push_in)
{
	std::pair<int, int> scroll_offsets;
	get_scroll_offsets(scroll_offsets.first, scroll_offsets.second);

	if (_auto_position_count > 1 && scroll_offsets != _auto_position_scroll_offsets) {
		// Scroll changed since last auto position, reset
		_menu_x              = 0;
		_menu_y              = 0;
		_auto_position_count = 0;
	}

	if (_menu_x == 0 && _menu_y == 0) {
		// No menu position set, start near top left of canvas
		widget().translate_coordinates(
			*_app.window_factory()->graph_window(_graph),
			64, 64, _menu_x, _menu_y);

		int origin_x = 0;
		int origin_y = 0;
		widget().get_window()->get_origin(origin_x, origin_y);
		_menu_x += origin_x;
		_menu_y += origin_y;
	}

	const int cascade = _auto_position_count * 32;

	x = _menu_x + cascade;
	y = _menu_y + cascade;
	push_in = true;

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
			_auto_position_count = 1;
			_menu_x = static_cast<int>(event->button.x_root);
			_menu_y = static_cast<int>(event->button.y_root);
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
		break;

	case GDK_MOTION_NOTIFY:
		_paste_count = 0;
		break;

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

	App*          app         = static_cast<App*>(data);
	Ganv::Module* module      = Glib::wrap(GANV_MODULE(node));
	auto*         node_module = dynamic_cast<NodeModule*>(module);

	if (node_module) {
		app->interface()->del(node_module->block()->uri());
	} else {
		auto* port_module = dynamic_cast<GraphPortModule*>(module);
		if (port_module &&
		    strcmp(port_module->port()->path().symbol(), "control") &&
		    strcmp(port_module->port()->path().symbol(), "notify")) {
			app->interface()->del(port_module->port()->uri());
		}
	}
}

static void
destroy_arc(GanvEdge* arc, void* data)
{
	App*        app   = static_cast<App*>(data);
	Ganv::Edge* arcmm = Glib::wrap(arc);

	Port* tail = dynamic_cast<Port*>(arcmm->get_tail());
	Port* head = dynamic_cast<Port*>(arcmm->get_head());
	app->interface()->disconnect(tail->model()->path(), head->model()->path());
}

void
GraphCanvas::destroy_selection()
{
	_app.interface()->bundle_begin();

	// TODO: Refine this to not destroy arcs that will be anyway?
	for_each_selected_edge(destroy_arc, &_app);
	for_each_selected_node(destroy_node, &_app);

	_app.interface()->bundle_end();
}

static void
serialise_node(GanvNode* node, void* data)
{
	auto* serialiser = static_cast<Serialiser*>(data);
	if (!GANV_IS_MODULE(node)) {
		return;
	}

	Ganv::Module* module      = Glib::wrap(GANV_MODULE(node));
	auto*         node_module = dynamic_cast<NodeModule*>(module);

	if (node_module) {
		serialiser->serialise(node_module->block());
	} else {
		auto* port_module = dynamic_cast<GraphPortModule*>(module);
		if (port_module) {
			serialiser->serialise(port_module->port());
		}
	}
}

static void
serialise_arc(GanvEdge* arc, void* data)
{
	auto* serialiser = static_cast<Serialiser*>(data);
	if (!GANV_IS_EDGE(arc)) {
		return;
	}

	auto* garc = dynamic_cast<gui::Arc*>(Glib::wrap(GANV_EDGE(arc)));
	if (garc) {
		serialiser->serialise_arc(Sord::Node(), garc->model());
	}
}

void
GraphCanvas::copy_selection()
{
	std::lock_guard<std::mutex> lock(_app.world().rdf_mutex());

	Serialiser serialiser(_app.world());
	serialiser.start_to_string(_graph->path(), _graph->base_uri());

	for_each_selected_node(serialise_node, &serialiser);
	for_each_selected_edge(serialise_arc, &serialiser);

	Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
	clipboard->set_text(serialiser.finish());
	_paste_count = 0;
}

void
GraphCanvas::paste()
{
	using PropIter = Properties::const_iterator;

	std::lock_guard<std::mutex> lock(_app.world().rdf_mutex());

	const Glib::ustring str    = Gtk::Clipboard::get()->wait_for_text();
	SPtr<Parser>        parser = _app.loader()->parser();
	const URIs&         uris   = _app.uris();
	const Raul::Path&   parent = _graph->path();
	if (!parser) {
		_app.log().error("Unable to load parser, paste unavailable\n");
		return;
	}

	// Prepare for paste
	clear_selection();
	_pastees.clear();
	++_paste_count;

	// Make a client store to serve as clipboard
	ClientStore clipboard(_app.world().uris(), _app.log());
	clipboard.set_plugins(_app.store()->plugins());
	clipboard.put(main_uri(),
	              {{uris.rdf_type, Property(uris.ingen_Graph)}});

	// Parse clipboard text into clipboard store
	boost::optional<URI> base_uri = parser->parse_string(
		_app.world(), clipboard, str, main_uri());

	// Figure out the copy graph base path
	Raul::Path copy_root("/");
	if (base_uri) {
		std::string base = *base_uri;
		if (base[base.size() - 1] == '/') {
			base = base.substr(0, base.size() - 1);
		}
		copy_root = uri_to_path(URI(base));
	}

	// Find the minimum x and y coordinate of objects to be pasted
	float min_x = std::numeric_limits<float>::max();
	float min_y = std::numeric_limits<float>::max();
	for (const auto& c : clipboard) {
		if (c.first.parent() == Raul::Path("/")) {
			const Atom& x = c.second->get_property(uris.ingen_canvasX);
			const Atom& y = c.second->get_property(uris.ingen_canvasY);
			if (x.type() == uris.atom_Float) {
				min_x = std::min(min_x, x.get<float>());
			}
			if (y.type() == uris.atom_Float) {
				min_y = std::min(min_y, y.get<float>());
			}
		}
	}

	// Find canvas paste origin based on pointer position
	int widget_point_x = 0;
	int widget_point_y = 0;
	int scroll_x       = 0;
	int scroll_y       = 0;
	widget().get_pointer(widget_point_x, widget_point_y);
	get_scroll_offsets(scroll_x, scroll_y);
	const int paste_x = widget_point_x + scroll_x + (20.0f * _paste_count);
	const int paste_y = widget_point_y + scroll_y + (20.0f * _paste_count);

	_app.interface()->bundle_begin();

	// Put each top level object in the clipboard store
	ClashAvoider avoider(*_app.store().get());
	for (const auto& c : clipboard) {
		if (c.first.is_root() || c.first.parent() != Raul::Path("/")) {
			continue;
		}

		const SPtr<Node>  node     = c.second;
		const Raul::Path& old_path = copy_root.child(node->path());
		const URI&        old_uri  = path_to_uri(old_path);
		const Raul::Path& new_path = avoider.map_path(parent.child(node->path()));

		// Copy properties, except those that should not be inherited in copies
		Properties props = node->properties();
		for (const auto& prop : {uris.lv2_prototype,
		                         uris.ingen_canvasX,
		                         uris.ingen_canvasY,
		                         uris.lv2_index,
		                         uris.lv2_symbol}) {
			props.erase(prop);
		}

		// Store the old URI as a prototype (keeps provenance around)
		props.emplace(uris.lv2_prototype, _app.forge().make_urid(old_uri));

		// Adjust numeric suffix on name if appropriate
		auto n = props.find(uris.lv2_name);
		if (n != props.end()) {
			n->second = _app.forge().alloc(ClashAvoider::adjust_name(
			        old_path, new_path, n->second.ptr<char>()));
		}

		// Set coordinates so paste origin is at the mouse pointer
		PropIter xi = node->properties().find(uris.ingen_canvasX);
		PropIter yi = node->properties().find(uris.ingen_canvasY);
		if (xi != node->properties().end()) {
			const float x = xi->second.get<float>() - min_x + paste_x;
			props.insert({xi->first, Property(_app.forge().make(x),
			                                  xi->second.context())});
		}
		if (yi != node->properties().end()) {
			const float y = yi->second.get<float>() - min_y + paste_y;
			props.insert({yi->first, Property(_app.forge().make(y),
			                                  yi->second.context())});
		}

		_app.interface()->put(path_to_uri(new_path), props);
		_pastees.insert(new_path);
	}

	// Connect objects
	for (const auto& a : clipboard.object(Raul::Path("/"))->arcs()) {
		_app.interface()->connect(
			avoider.map_path(parent.child(a.second->tail_path())),
			avoider.map_path(parent.child(a.second->head_path())));
	}

	_app.interface()->bundle_end();
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
		if (!_graph->get_port(Raul::Symbol::symbolify(symbol))) {
			break;
		}
	}

	assert(Raul::Path::is_valid(string("/") + symbol));

	name.append(" ").append(num_buf);
}

void
GraphCanvas::menu_add_port(const string& sym_base,
                           const string& name_base,
                           const URI&    type,
                           bool          is_output)
{
	string sym;
	string name;
	generate_port_name(sym_base, sym, name_base, name);
	const Raul::Path& path = _graph->path().child(Raul::Symbol(sym));

	const URIs& uris = _app.uris();

	Properties props = get_initial_data(Resource::Graph::INTERNAL);
	props.emplace(uris.rdf_type, _app.forge().make_urid(type));
	if (type == uris.atom_AtomPort) {
		props.emplace(uris.atom_bufferType, Property(uris.atom_Sequence));
	}
	props.emplace(
		uris.rdf_type,
		Property(is_output ? uris.lv2_OutputPort : uris.lv2_InputPort));
	props.emplace(uris.lv2_index,
	              _app.forge().make(int32_t(_graph->num_ports())));
	props.emplace(uris.lv2_name, _app.forge().alloc(name.c_str()));
	_app.interface()->put(path_to_uri(path), props);
}

void
GraphCanvas::load_plugin(const WPtr<PluginModel>& weak_plugin)
{
	SPtr<PluginModel> plugin = weak_plugin.lock();
	if (!plugin) {
		return;
	}

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
	Properties props = get_initial_data();
	props.emplace(uris.rdf_type, Property(uris.ingen_Block));
	props.emplace(uris.lv2_prototype, uris.forge.make_urid(plugin->uri()));
	_app.interface()->put(path_to_uri(path), props);
}

/** Try to guess a suitable location for a new module.
 */
void
GraphCanvas::get_new_module_location(double& x, double& y)
{
	int scroll_x = 0;
	int scroll_y = 0;
	get_scroll_offsets(scroll_x, scroll_y);
	x = scroll_x + 20;
	y = scroll_y + 20;
}

Properties
GraphCanvas::get_initial_data(Resource::Graph ctx)
{
	Properties result;
	const URIs& uris = _app.uris();
	result.emplace(uris.ingen_canvasX,
	               Property(_app.forge().make(static_cast<float>(_menu_x)),
	                        ctx));
	result.emplace(uris.ingen_canvasY,
	               Property(_app.forge().make(static_cast<float>(_menu_y)),
	                        ctx));
	return result;
}

void
GraphCanvas::menu_load_plugin()
{
	_app.window_factory()->present_load_plugin(_graph, get_initial_data());
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

} // namespace gui
} // namespace ingen
