/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#include <cassert>
#include <map>
#include <string>
#include "raul/log.hpp"
#include "flowcanvas/Canvas.hpp"
#include "flowcanvas/Ellipse.hpp"
#include "interface/EngineInterface.hpp"
#include "shared/LV2URIMap.hpp"
#include "shared/Builder.hpp"
#include "shared/ClashAvoider.hpp"
#include "serialisation/Serialiser.hpp"
#include "client/PluginModel.hpp"
#include "client/PatchModel.hpp"
#include "client/NodeModel.hpp"
#include "client/ClientStore.hpp"
#include "module/World.hpp"
#include "App.hpp"
#include "PatchCanvas.hpp"
#include "PatchWindow.hpp"
#include "PatchPortModule.hpp"
#include "LoadPluginWindow.hpp"
#include "NewSubpatchWindow.hpp"
#include "Port.hpp"
#include "Connection.hpp"
#include "NodeModule.hpp"
#include "SubpatchModule.hpp"
#include "GladeFactory.hpp"
#include "WindowFactory.hpp"
#include "ThreadedLoader.hpp"
#include "ingen-config.h"

#define LOG(s) s << "[PatchCanvas] "

using Ingen::Client::ClientStore;
using Ingen::Serialisation::Serialiser;
using Ingen::Client::PluginModel;
using namespace std;
using namespace Raul;

namespace Ingen {
namespace GUI {


PatchCanvas::PatchCanvas(SharedPtr<PatchModel> patch, int width, int height)
	: Canvas(width, height)
	, _patch(patch)
	, _last_click_x(0)
	, _last_click_y(0)
	, _paste_count(0)
	, _human_names(true)
	, _show_port_names(true)
	, _menu(NULL)
	, _internal_menu(NULL)
	, _classless_menu(NULL)
	, _plugin_menu(NULL)
{
	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference();
	xml->get_widget("canvas_menu", _menu);

	xml->get_widget("canvas_menu_add_audio_input", _menu_add_audio_input);
	xml->get_widget("canvas_menu_add_audio_output", _menu_add_audio_output);
	xml->get_widget("canvas_menu_add_control_input", _menu_add_control_input);
	xml->get_widget("canvas_menu_add_control_output", _menu_add_control_output);
	xml->get_widget("canvas_menu_add_event_input", _menu_add_event_input);
	xml->get_widget("canvas_menu_add_event_output", _menu_add_event_output);
	xml->get_widget("canvas_menu_load_plugin", _menu_load_plugin);
	xml->get_widget("canvas_menu_load_patch", _menu_load_patch);
	xml->get_widget("canvas_menu_new_patch", _menu_new_patch);
	xml->get_widget("canvas_menu_edit", _menu_edit);

	// Add port menu items
	_menu_add_audio_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"audio_in", "Audio In", "http://lv2plug.in/ns/lv2core#AudioPort", false));
	_menu_add_audio_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"audio_out", "Audio Out", "http://lv2plug.in/ns/lv2core#AudioPort", true));
	_menu_add_control_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"control_in", "Control In", "http://lv2plug.in/ns/lv2core#ControlPort", false));
	_menu_add_control_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"control_out", "Control Out", "http://lv2plug.in/ns/lv2core#ControlPort", true));
	_menu_add_event_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"event_in", "Event In", "http://lv2plug.in/ns/ext/event#EventPort", false));
	_menu_add_event_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"event_out", "Event Out", "http://lv2plug.in/ns/ext/event#EventPort", true));

	// Connect to model signals to track state
	_patch->signal_new_node.connect(sigc::mem_fun(this, &PatchCanvas::add_node));
	_patch->signal_removed_node.connect(sigc::mem_fun(this, &PatchCanvas::remove_node));
	_patch->signal_new_port.connect(sigc::mem_fun(this, &PatchCanvas::add_port));
	_patch->signal_removed_port.connect(sigc::mem_fun(this, &PatchCanvas::remove_port));
	_patch->signal_new_connection.connect(sigc::mem_fun(this, &PatchCanvas::connection));
	_patch->signal_removed_connection.connect(sigc::mem_fun(this, &PatchCanvas::disconnection));

	App::instance().store()->signal_new_plugin.connect(sigc::mem_fun(this, &PatchCanvas::add_plugin));

	// Connect widget signals to do things
	_menu_load_plugin->signal_activate().connect(sigc::mem_fun(this, &PatchCanvas::menu_load_plugin));
	_menu_load_patch->signal_activate().connect(sigc::mem_fun(this, &PatchCanvas::menu_load_patch));
	_menu_new_patch->signal_activate().connect(sigc::mem_fun(this, &PatchCanvas::menu_new_patch));
	_menu_edit->signal_activate().connect(sigc::mem_fun(this, &PatchCanvas::menu_edit_toggled));

	_patch->signal_editable.connect(sigc::mem_fun(this, &PatchCanvas::patch_editable_changed));
}


void
PatchCanvas::show_menu(GdkEvent* event)
{
	if (!_internal_menu)
		build_menus();

	_menu->popup(event->button.button, event->button.time);
}


void
PatchCanvas::build_menus()
{
	// Build (or clear existing) internal plugin menu
	if (_internal_menu) {
		_internal_menu->items().clear();
	} else {
		_menu->items().push_back(Gtk::Menu_Helpers::ImageMenuElem("I_nternal",
				*(manage(new Gtk::Image(Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_MENU)))));
		Gtk::MenuItem* internal_menu_item = &(_menu->items().back());
		_internal_menu = Gtk::manage(new Gtk::Menu());
		internal_menu_item->set_submenu(*_internal_menu);
		_menu->reorder_child(*internal_menu_item, 4);
	}

	// Build skeleton LV2 plugin class heirarchy for 'Plugin' menu
#ifdef HAVE_SLV2
	if (!_plugin_menu)
		build_plugin_menu();

	// Build (or clear existing) uncategorized (classless, heh) plugins menu
	if (_classless_menu) {
		_classless_menu->items().clear();
	} else {
		_plugin_menu->items().push_back(Gtk::Menu_Helpers::MenuElem("Uncategorized"));
		Gtk::MenuItem* classless_menu_item = &(_plugin_menu->items().back());
		_classless_menu = Gtk::manage(new Gtk::Menu());
		classless_menu_item->set_submenu(*_classless_menu);
		_classless_menu->hide();
	}
#endif

	// Add known plugins to menu heirarchy
	SharedPtr<const ClientStore::Plugins> plugins = App::instance().store()->plugins();
	for (ClientStore::Plugins::const_iterator i = plugins->begin(); i != plugins->end(); ++i)
		add_plugin(i->second);
}

#ifdef HAVE_SLV2

/** Recursively build the plugin class menu heirarchy rooted at
 * @a plugin class into @a menu
 */
size_t
PatchCanvas::build_plugin_class_menu(Gtk::Menu* menu,
		SLV2PluginClass plugin_class, SLV2PluginClasses classes, const LV2Children& children)
{
	size_t      num_items     = 0;
	SLV2Value   class_uri     = slv2_plugin_class_get_uri(plugin_class);
	const char* class_uri_str = slv2_value_as_string(class_uri);

	const std::pair<LV2Children::const_iterator, LV2Children::const_iterator> kids
			= children.equal_range(class_uri_str);

	if (kids.first == children.end())
		return 0;

	// Add submenus
	for (LV2Children::const_iterator i = kids.first; i != kids.second; ++i) {
		SLV2PluginClass c = i->second;
		const char* sub_label_str = slv2_value_as_string(slv2_plugin_class_get_label(c));
		const char* sub_uri_str   = slv2_value_as_string(slv2_plugin_class_get_uri(c));

		Gtk::Menu_Helpers::MenuElem menu_elem = Gtk::Menu_Helpers::MenuElem(sub_label_str);
		menu->items().push_back(menu_elem);
		Gtk::MenuItem* menu_item = &(menu->items().back());

		Gtk::Menu* submenu = Gtk::manage(new Gtk::Menu());
		menu_item->set_submenu(*submenu);

		size_t num_child_items = build_plugin_class_menu(submenu, c, classes, children);

		_class_menus.insert(make_pair(sub_uri_str, MenuRecord(menu_item, submenu)));
		if (num_child_items == 0)
			menu_item->hide();

		++num_items;
	}

	return num_items;
}


void
PatchCanvas::build_plugin_menu()
{
	if (_plugin_menu) {
		_plugin_menu->items().clear();
	} else {
		_menu->items().push_back(Gtk::Menu_Helpers::ImageMenuElem("_Plugin",
				*(manage(new Gtk::Image(Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_MENU)))));
		Gtk::MenuItem* plugin_menu_item = &(_menu->items().back());
		_plugin_menu = Gtk::manage(new Gtk::Menu());
		plugin_menu_item->set_submenu(*_plugin_menu);
		_menu->reorder_child(*plugin_menu_item, 5);
	}

	Glib::Mutex::Lock lock(PluginModel::rdf_world()->mutex());
	SLV2PluginClass lv2_plugin = slv2_world_get_plugin_class(PluginModel::slv2_world());
	SLV2PluginClasses classes = slv2_world_get_plugin_classes(PluginModel::slv2_world());

	LV2Children children;
	for (unsigned i=0; i < slv2_plugin_classes_size(classes); ++i) {
		SLV2PluginClass c = slv2_plugin_classes_get_at(classes, i);
		SLV2Value       p = slv2_plugin_class_get_parent_uri(c);
		if (!p)
			p = slv2_plugin_class_get_uri(lv2_plugin);
		children.insert(make_pair(slv2_value_as_string(p), c));
	}
	build_plugin_class_menu(_plugin_menu, lv2_plugin, classes, children);
}

#endif

void
PatchCanvas::build()
{
	boost::shared_ptr<PatchCanvas> shared_this =
		boost::dynamic_pointer_cast<PatchCanvas>(shared_from_this());

	// Create modules for nodes
	for (Store::const_iterator i = App::instance().store()->children_begin(_patch);
			i != App::instance().store()->children_end(_patch); ++i) {
		SharedPtr<NodeModel> node = PtrCast<NodeModel>(i->second);
		if (node && node->parent() == _patch)
			add_node(node);
	}

	// Create pseudo modules for ports (ports on this canvas, not on our module)
	for (NodeModel::Ports::const_iterator i = _patch->ports().begin();
			i != _patch->ports().end(); ++i) {
		add_port(*i);
	}

	// Create connections
	for (PatchModel::Connections::const_iterator i = _patch->connections().begin();
			i != _patch->connections().end(); ++i) {
		connection(PtrCast<ConnectionModel>(*i));
	}
}


void
PatchCanvas::arrange(bool ingen_doesnt_use_length_hints)
{
	FlowCanvas::Canvas::arrange(false);

	for (list<boost::shared_ptr<Item> >::iterator i = _items.begin(); i != _items.end(); ++i)
		(*i)->store_location();
}


void
PatchCanvas::show_human_names(bool b)
{
	_human_names = b;
	for (ItemList::iterator m = _items.begin(); m != _items.end(); ++m) {
		boost::shared_ptr<NodeModule> mod = boost::dynamic_pointer_cast<NodeModule>(*m);
		if (mod)
			mod->show_human_names(b);

		boost::shared_ptr<PatchPortModule> pmod = boost::dynamic_pointer_cast<PatchPortModule>(*m);
		if (pmod)
			pmod->show_human_names(b);
	}
}


void
PatchCanvas::show_port_names(bool b)
{
	_show_port_names = b;
	for (ItemList::iterator i = _items.begin(); i != _items.end(); ++i) {
		boost::shared_ptr<FlowCanvas::Module> m = boost::dynamic_pointer_cast<FlowCanvas::Module>(*i);
		if (m)
			m->set_show_port_labels(b);
	}
}


void
PatchCanvas::add_plugin(SharedPtr<PluginModel> p)
{
	typedef ClassMenus::iterator iterator;
	if (_internal_menu && p->type() == Plugin::Internal) {
		_internal_menu->items().push_back(Gtk::Menu_Helpers::MenuElem(p->human_name(),
				sigc::bind(sigc::mem_fun(this, &PatchCanvas::load_plugin), p)));
	} else if (_plugin_menu && p->type() == Plugin::LV2 && p->slv2_plugin()) {
		SLV2PluginClass pc            = slv2_plugin_get_class(p->slv2_plugin());
		SLV2Value       class_uri     = slv2_plugin_class_get_uri(pc);
		const char*     class_uri_str = slv2_value_as_string(class_uri);

		Glib::RefPtr<Gdk::Pixbuf> icon = App::instance().icon_from_path(
					PluginModel::get_lv2_icon_path(p->slv2_plugin()), 16);

		pair<iterator,iterator> range = _class_menus.equal_range(class_uri_str);
		if (range.first == _class_menus.end() || range.first == range.second
				|| range.first->second.menu == _plugin_menu) {
			_classless_menu->items().push_back(Gtk::Menu_Helpers::MenuElem(p->human_name(),
					sigc::bind(sigc::mem_fun(this, &PatchCanvas::load_plugin), p)));
			if (!_classless_menu->is_visible())
				_classless_menu->show();
		} else {
			// For each menu that represents plugin's class (possibly several)
			for (iterator i = range.first; i != range.second ; ++i) {
				Gtk::Menu* menu = i->second.menu;
				if (icon) {
					Gtk::Image* image = new Gtk::Image(icon);
					menu->items().push_back(Gtk::Menu_Helpers::ImageMenuElem(p->human_name(),
							*image,
							sigc::bind(sigc::mem_fun(this, &PatchCanvas::load_plugin), p)));
				} else {
					menu->items().push_back(Gtk::Menu_Helpers::MenuElem(p->human_name(),
							sigc::bind(sigc::mem_fun(this, &PatchCanvas::load_plugin), p)));
				}
				if (!i->second.item->is_visible())
					i->second.item->show();
			}
		}
	}
}


void
PatchCanvas::add_node(SharedPtr<NodeModel> nm)
{
	boost::shared_ptr<PatchCanvas> shared_this =
		boost::dynamic_pointer_cast<PatchCanvas>(shared_from_this());

	SharedPtr<PatchModel> pm = PtrCast<PatchModel>(nm);
	SharedPtr<NodeModule> module;
	if (pm) {
		module = SubpatchModule::create(shared_this, pm, _human_names);
	} else {
		module = NodeModule::create(shared_this, nm, _human_names);
		const PluginModel* plugm = dynamic_cast<const PluginModel*>(nm->plugin());
		if (plugm && !plugm->icon_path().empty())
			module->set_icon(App::instance().icon_from_path(plugm->icon_path(), 100));
	}

	add_item(module);
	module->show();
	_views.insert(std::make_pair(nm, module));
}


void
PatchCanvas::remove_node(SharedPtr<NodeModel> nm)
{
	Views::iterator i = _views.find(nm);

	if (i != _views.end()) {
		remove_item(i->second);
		_views.erase(i);
	}
}


void
PatchCanvas::add_port(SharedPtr<PortModel> pm)
{
	boost::shared_ptr<PatchCanvas> shared_this =
		boost::dynamic_pointer_cast<PatchCanvas>(shared_from_this());

	SharedPtr<PatchPortModule> view = PatchPortModule::create(shared_this, pm, _human_names);
	_views.insert(std::make_pair(pm, view));
	add_item(view);
	view->show();
}


void
PatchCanvas::remove_port(SharedPtr<PortModel> pm)
{
	Views::iterator i = _views.find(pm);

	// Port on this patch
	if (i != _views.end()) {
		bool ret = remove_item(i->second);
		if (!ret)
			warn << "Failed to remove port item " << pm->path() << endl;
		i->second.reset();
		_views.erase(i);

	} else {
		SharedPtr<NodeModule> module = PtrCast<NodeModule>(_views[pm->parent()]);
		module->remove_port(pm);
	}

	assert(_views.find(pm) == _views.end());
}


SharedPtr<FlowCanvas::Port>
PatchCanvas::get_port_view(SharedPtr<PortModel> port)
{
	SharedPtr<FlowCanvas::Module> module = _views[port];

	// Port on this patch
	if (module) {
		return (PtrCast<PatchPortModule>(module))
			? *(PtrCast<PatchPortModule>(module)->ports().begin())
			: PtrCast<FlowCanvas::Port>(module);
	} else {
		module = PtrCast<NodeModule>(_views[port->parent()]);
		if (module) {
			for (PortVector::const_iterator p = module->ports().begin();
					p != module->ports().end(); ++p) {
				boost::shared_ptr<GUI::Port> pv = boost::dynamic_pointer_cast<GUI::Port>(*p);
				if (pv && pv->model() == port)
					return pv;
			}
		}
	}

	return SharedPtr<FlowCanvas::Port>();
}


void
PatchCanvas::connection(SharedPtr<ConnectionModel> cm)
{
	assert(cm);

	const SharedPtr<FlowCanvas::Port> src = get_port_view(cm->src_port());
	const SharedPtr<FlowCanvas::Port> dst = get_port_view(cm->dst_port());

	if (src && dst) {
		add_connection(boost::shared_ptr<GUI::Connection>(new GUI::Connection(shared_from_this(),
				cm, src, dst, src->color() + 0x22222200)));
	} else {
		LOG(error) << "Unable to find ports to connect "
			<< cm->src_port_path() << " -> " << cm->dst_port_path() << endl;
	}
}


void
PatchCanvas::disconnection(SharedPtr<ConnectionModel> cm)
{
	const SharedPtr<FlowCanvas::Port> src = get_port_view(cm->src_port());
	const SharedPtr<FlowCanvas::Port> dst = get_port_view(cm->dst_port());

	if (src && dst)
		remove_connection(src, dst);
	else
		LOG(error) << "Unable to find ports to disconnect "
			<< cm->src_port_path() << " -> " << cm->dst_port_path() << endl;
}


void
PatchCanvas::connect(boost::shared_ptr<FlowCanvas::Connectable> src_port,
                     boost::shared_ptr<FlowCanvas::Connectable> dst_port)
{
	const boost::shared_ptr<Ingen::GUI::Port> src
		= boost::dynamic_pointer_cast<Ingen::GUI::Port>(src_port);

	const boost::shared_ptr<Ingen::GUI::Port> dst
		= boost::dynamic_pointer_cast<Ingen::GUI::Port>(dst_port);

	if (!src || !dst)
		return;

	// Midi binding/learn shortcut
	if (src->model()->type().is_events() && dst->model()->type().is_control()) {
		LOG(error) << "TODO: MIDI binding shortcut" << endl;
	} else {
		App::instance().engine()->connect(src->model()->path(), dst->model()->path());
	}
}


void
PatchCanvas::disconnect(boost::shared_ptr<FlowCanvas::Connectable> src_port,
                        boost::shared_ptr<FlowCanvas::Connectable> dst_port)
{
	const boost::shared_ptr<Ingen::GUI::Port> src
		= boost::dynamic_pointer_cast<Ingen::GUI::Port>(src_port);

	const boost::shared_ptr<Ingen::GUI::Port> dst
		= boost::dynamic_pointer_cast<Ingen::GUI::Port>(dst_port);

	App::instance().engine()->disconnect(src->model()->path(),
	                                     dst->model()->path());
}


bool
PatchCanvas::canvas_event(GdkEvent* event)
{
	assert(event);

	bool ret = false;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 3) {
			_last_click_x = (int)event->button.x;
			_last_click_y = (int)event->button.y;
			show_menu(event);
			ret = true;
		}
		break;

	case GDK_KEY_PRESS:
	case GDK_KEY_RELEASE:
		ret = canvas_key_event(&event->key);

	default:
		break;
	}

	return (ret ? true : Canvas::canvas_event(event));
}


bool
PatchCanvas::canvas_key_event(GdkEventKey* event)
{
	switch (event->type) {
	case GDK_KEY_PRESS:
		switch (event->keyval) {
		case GDK_Delete:
			destroy_selection();
			return true;
		case GDK_e:
			if (event->state == 0) {
				_patch->set_editable(!_patch->get_editable());
				return true;
			} else {
				return false;
			}
		default:
			return false;
		}
	default:
		return false;
	}
}


void
PatchCanvas::destroy_selection()
{
	for (list<boost::shared_ptr<Item> >::iterator m = _selected_items.begin(); m != _selected_items.end(); ++m) {
		boost::shared_ptr<NodeModule> module = boost::dynamic_pointer_cast<NodeModule>(*m);
		if (module) {
			App::instance().engine()->del(module->node()->path());
		} else {
			boost::shared_ptr<PatchPortModule> port_module = boost::dynamic_pointer_cast<PatchPortModule>(*m);
			if (port_module)
				App::instance().engine()->del(port_module->port()->path());
		}
	}
}

void
PatchCanvas::select_all()
{
	unselect_ports();
	for (list<boost::shared_ptr<Item> >::iterator m = _items.begin(); m != _items.end(); ++m)
		if (boost::dynamic_pointer_cast<FlowCanvas::Module>(*m))
			if (!(*m)->selected())
				select_item(*m);
}

void
PatchCanvas::copy_selection()
{
	Serialiser serialiser(*App::instance().world(), App::instance().store());
	serialiser.start_to_string(_patch->path(), "http://example.org/");

	for (list<boost::shared_ptr<Item> >::iterator m = _selected_items.begin(); m != _selected_items.end(); ++m) {
		boost::shared_ptr<NodeModule> module = boost::dynamic_pointer_cast<NodeModule>(*m);
		if (module) {
			serialiser.serialise(module->node());
		} else {
			boost::shared_ptr<PatchPortModule> port_module = boost::dynamic_pointer_cast<PatchPortModule>(*m);
			if (port_module)
				serialiser.serialise(port_module->port());
		}
	}

	for (list<boost::shared_ptr<FlowCanvas::Connection> >::iterator c = _selected_connections.begin();
			c != _selected_connections.end(); ++c) {
		boost::shared_ptr<Connection> connection = boost::dynamic_pointer_cast<Connection>(*c);
		if (connection)
			serialiser.serialise_connection(_patch, connection->model());
	}

	string result = serialiser.finish();
	_paste_count = 0;

	Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
	clipboard->set_text(result);
}


void
PatchCanvas::paste()
{
	Glib::ustring str = Gtk::Clipboard::get()->wait_for_text();
	SharedPtr<Parser> parser = App::instance().loader()->parser();
	if (!parser) {
		LOG(error) << "Unable to load parser, paste unavailable" << endl;
		return;
	}

	clear_selection();
	++_paste_count;

	Builder builder(*App::instance().engine());
	ClientStore clipboard;
	clipboard.set_plugins(App::instance().store()->plugins());

	const LV2URIMap& uris = App::instance().uris();

	// mkdir -p
	string to_create = _patch->path().chop_scheme().substr(1);
	string created = "/";
	Resource::Properties props;
	props.insert(make_pair(uris.rdf_type,        uris.ingen_Patch));
	props.insert(make_pair(uris.ingen_polyphony, Raul::Atom(int32_t(_patch->poly()))));
	clipboard.put(Path(), props);
	size_t first_slash;
	while (to_create != "/" && !to_create.empty()
			&& (first_slash = to_create.find("/")) != string::npos) {
		created += to_create.substr(0, first_slash);
		assert(Path::is_valid(created));
		clipboard.put(created, props);
		to_create = to_create.substr(first_slash + 1);
	}

	if (!_patch->path().is_root())
		clipboard.put(_patch->path(), props);

	boost::optional<Raul::Path>   data_path;
	boost::optional<Raul::Path>   parent;
	boost::optional<Raul::Symbol> symbol;

	if (!_patch->path().is_root()) {
		parent = _patch->path();
	}

	ClashAvoider avoider(*App::instance().store().get(), clipboard, &clipboard);
	parser->parse_string(App::instance().world(), &avoider, str, Path().str(), data_path,
			parent, symbol);

	for (Store::iterator i = clipboard.begin(); i != clipboard.end(); ++i) {
		if (_patch->path().is_root() && i->first.is_root())
			continue;
		GraphObject::Properties::iterator x = i->second->properties().find(uris.ingenui_canvas_x);
		if (x != i->second->properties().end())
			x->second = x->second.get_float() + (20.0f * _paste_count);
		GraphObject::Properties::iterator y = i->second->properties().find(uris.ingenui_canvas_y);
		if (y != i->second->properties().end())
			y->second = y->second.get_float() + (20.0f * _paste_count);
		if (i->first.parent().is_root()) {
			GraphObject::Properties::iterator s = i->second->properties().find(uris.ingen_selected);
			if (s != i->second->properties().end())
				s->second = true;
			else
				i->second->properties().insert(make_pair(uris.ingen_selected, true));
		}
		builder.build(i->second);
	}

	// Successful connections
	SharedPtr<PatchModel> root = PtrCast<PatchModel>(clipboard.object(_patch->path()));
	assert(root);
	for (Patch::Connections::const_iterator i = root->connections().begin();
			i != root->connections().end(); ++i) {
		App::instance().engine()->connect((*i)->src_port_path(), (*i)->dst_port_path());
	}
}


void
PatchCanvas::generate_port_name(
		const string& sym_base,  string& symbol,
		const string& name_base, string& name)
{
	symbol = sym_base;
	name   = name_base;

	char num_buf[5];
	uint32_t i = 1;
	for ( ; i < 9999; ++i) {
		snprintf(num_buf, 5, "%u", i);
		symbol = sym_base + "_";
		symbol += num_buf;
		if (!_patch->get_port(symbol))
			break;
	}

	assert(Path::is_valid(string("/") + symbol));

	name.append(" ").append(num_buf);
}


void
PatchCanvas::menu_add_port(const string& sym_base, const string& name_base,
		const Raul::URI& type, bool is_output)
{
	string sym, name;
	generate_port_name(sym_base, sym, name_base, name);
	const Path& path = _patch->path().base() + sym;

	const LV2URIMap& uris = App::instance().uris();

	Resource::Properties props = get_initial_data();
	props.insert(make_pair(uris.rdf_type, type));
	props.insert(make_pair(uris.rdf_type, is_output ? uris.lv2_OutputPort : uris.lv2_InputPort));
	props.insert(make_pair(uris.lv2_index, Atom(int32_t(_patch->num_ports()))));
	props.insert(make_pair(uris.lv2_name, Atom(name.c_str())));
	App::instance().engine()->put(path, props);
}


void
PatchCanvas::load_plugin(WeakPtr<PluginModel> weak_plugin)
{
	SharedPtr<PluginModel> plugin = weak_plugin.lock();
	if (!plugin)
		return;

	Raul::Symbol symbol = plugin->default_node_symbol();
	unsigned offset = App::instance().store()->child_name_offset(_patch->path(), symbol);
	if (offset != 0) {
		std::stringstream ss;
		ss << symbol << "_" << offset;
		symbol = ss.str();
	}

	const LV2URIMap& uris = App::instance().uris();

	const Path path = _patch->path().child(symbol);
	// FIXME: polyphony?
	GraphObject::Properties props = get_initial_data();
	props.insert(make_pair(uris.rdf_type, uris.ingen_Node));
	props.insert(make_pair(uris.rdf_instanceOf, plugin->uri()));
	App::instance().engine()->put(path, props);
}


/** Try to guess a suitable location for a new module.
 */
void
PatchCanvas::get_new_module_location(double& x, double& y)
{
	int scroll_x;
	int scroll_y;
	get_scroll_offsets(scroll_x, scroll_y);
	x = scroll_x + 20;
	y = scroll_y + 20;
}


GraphObject::Properties
PatchCanvas::get_initial_data()
{
	GraphObject::Properties result;
	const LV2URIMap& uris = App::instance().uris();
	result.insert(make_pair(uris.ingenui_canvas_x, Atom((float)_last_click_x)));
	result.insert(make_pair(uris.ingenui_canvas_y, Atom((float)_last_click_y)));
	return result;
}

void
PatchCanvas::menu_load_plugin()
{
	App::instance().window_factory()->present_load_plugin(_patch, get_initial_data());
}


void
PatchCanvas::menu_load_patch()
{
	App::instance().window_factory()->present_load_subpatch(_patch, get_initial_data());
}


void
PatchCanvas::menu_new_patch()
{
	App::instance().window_factory()->present_new_subpatch(_patch, get_initial_data());
}


void
PatchCanvas::menu_edit_toggled()
{
	_patch->set_editable(_menu_edit->get_active());
}


void
PatchCanvas::patch_editable_changed(bool editable)
{
	if (_menu_edit->get_active() != editable)
		_menu_edit->set_active(editable);
}


} // namespace GUI
} // namespace Ingen
