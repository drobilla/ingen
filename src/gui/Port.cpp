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
#include <string>

#include "ganv/Module.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/client/GraphModel.hpp"
#include "ingen/client/PortModel.hpp"

#include "App.hpp"
#include "GraphWindow.hpp"
#include "Port.hpp"
#include "PortMenu.hpp"
#include "RDFS.hpp"
#include "Style.hpp"
#include "WidgetFactory.hpp"
#include "WindowFactory.hpp"
#include "ingen_config.h"

using namespace Ingen::Client;
using namespace std;

namespace Ingen {
namespace GUI {

Port*
Port::create(App&                  app,
             Ganv::Module&         module,
             SPtr<const PortModel> pm,
             bool                  flip)
{
	return new Port(app, module, pm, port_label(app, pm), flip);
}

/** @param flip Make an input port appear as an output port, and vice versa.
 */
Port::Port(App&                  app,
           Ganv::Module&         module,
           SPtr<const PortModel> pm,
           const string&         name,
           bool                  flip)
	: Ganv::Port(module, name,
	             flip ? (!pm->is_input()) : pm->is_input(),
	             app.style()->get_port_color(pm.get()))
	, _app(app)
	, _port_model(pm)
	, _entered(false)
	, _flipped(flip)
{
	assert(pm);

	if (app.can_control(pm.get())) {
		show_control();
		pm->signal_property().connect(
			sigc::mem_fun(this, &Port::property_changed));
		pm->signal_property_removed().connect(
			sigc::mem_fun(this, &Port::property_removed));
		pm->signal_value_changed().connect(
			sigc::mem_fun(this, &Port::value_changed));
	}

	port_properties_changed();

	pm->signal_activity().connect(
		sigc::mem_fun(this, &Port::activity));
	pm->signal_disconnection().connect(
		sigc::mem_fun(this, &Port::disconnected_from));
	pm->signal_moved().connect(
		sigc::mem_fun(this, &Port::moved));

	signal_value_changed.connect(
		sigc::mem_fun(this, &Port::on_value_changed));

	signal_event().connect(
		sigc::mem_fun(this, &Port::on_event));

	set_is_controllable(pm->is_numeric() && pm->is_input());

	update_metadata();
	value_changed(pm->value());
}

Port::~Port()
{
	_app.activity_port_destroyed(this);
}

std::string
Port::port_label(App& app, SPtr<const PortModel> pm)
{
	if (!pm) {
		return "";
	}

	std::string label;
	if (app.world()->conf().option("port-labels").get<int32_t>()) {
		if (app.world()->conf().option("human-names").get<int32_t>()) {
			const Atom& name = pm->get_property(app.uris().lv2_name);
			if (name.type() == app.forge().String) {
				label = name.ptr<char>();
			} else {
				const SPtr<const BlockModel> parent(
					dynamic_ptr_cast<const BlockModel>(pm->parent()));
				if (parent && parent->plugin_model())
					label = parent->plugin_model()->port_human_name(pm->index());
			}
		} else {
			label = pm->path().symbol();
		}
	}
	return label;
}

void
Port::ensure_label()
{
	if (!get_label()) {
		set_label(port_label(_app, _port_model.lock()).c_str());
	}
}

void
Port::update_metadata()
{
	SPtr<const PortModel> pm = _port_model.lock();
	if (pm && _app.can_control(pm.get()) && pm->is_numeric()) {
		SPtr<const BlockModel> parent = dynamic_ptr_cast<const BlockModel>(pm->parent());
		if (parent) {
			float min = 0.0f;
			float max = 1.0f;
			parent->port_value_range(pm, min, max, _app.sample_rate());
			set_control_min(min);
			set_control_max(max);
		}
	}
}

bool
Port::show_menu(GdkEventButton* ev)
{
	PortMenu* menu = NULL;
	WidgetFactory::get_widget_derived("object_menu", menu);
	if (!menu) {
		_app.log().error("Failed to load port menu widget\n");
		return false;
	}

	menu->init(_app, model(), _flipped);
	menu->popup(ev->button, ev->time);
	return true;
}

void
Port::moved()
{
	if (_app.world()->conf().option("port-labels").get<int32_t>() &&
	    !_app.world()->conf().option("human-names").get<int32_t>()) {
		set_label(model()->symbol().c_str());
	}
}

void
Port::on_value_changed(double value)
{
	const URIs& uris          = _app.uris();
	const Atom& current_value = model()->value();
	if (current_value.type() != uris.forge.Float) {
		return;  // Non-float, unsupported
	}

	if (current_value.get<float>() == (float)value) {
		return;  // No change
	}

	const Atom atom = _app.forge().make(float(value));
	_app.set_property(model()->uri(),
	                  _app.world()->uris().ingen_value,
	                  atom);

	if (_entered) {
		GraphBox* box = get_graph_box();
		if (box) {
			box->show_port_status(model().get(), atom);
		}
	}
}

void
Port::value_changed(const Atom& value)
{
	if (value.type() == _app.forge().Float && !get_grabbed()) {
		Ganv::Port::set_control_value(value.get<float>());
	}
}

void
Port::on_scale_point_activated(float f)
{
	_app.set_property(model()->uri(),
	                  _app.world()->uris().ingen_value,
	                  _app.world()->forge().make(f));
}

Gtk::Menu*
Port::build_enum_menu()
{
	SPtr<const BlockModel> block = dynamic_ptr_cast<BlockModel>(model()->parent());
	Gtk::Menu*             menu  = Gtk::manage(new Gtk::Menu());

	PluginModel::ScalePoints points = block->plugin_model()->port_scale_points(
		model()->index());
	for (PluginModel::ScalePoints::iterator i = points.begin();
	     i != points.end(); ++i) {
		menu->items().push_back(Gtk::Menu_Helpers::MenuElem(i->second));
		Gtk::MenuItem* menu_item = &(menu->items().back());
		menu_item->signal_activate().connect(
			sigc::bind(sigc::mem_fun(this, &Port::on_scale_point_activated),
			           i->first));
	}

	return menu;
}

void
Port::on_uri_activated(const Raul::URI& uri)
{
	_app.set_property(model()->uri(),
	                  _app.world()->uris().ingen_value,
	                  _app.world()->forge().make_urid(
		                  _app.world()->uri_map().map_uri(uri.c_str())));
}

Gtk::Menu*
Port::build_uri_menu()
{
	World*                 world = _app.world();
	SPtr<const BlockModel> block = dynamic_ptr_cast<BlockModel>(model()->parent());
	Gtk::Menu*             menu  = Gtk::manage(new Gtk::Menu());

	// Get the port designation, which should be a rdf:Property
	const Atom& designation_atom = model()->get_property(
		_app.uris().lv2_designation);
	if (!designation_atom.is_valid()) {
		return NULL;
	}

	LilvNode* designation = lilv_new_uri(
		world->lilv_world(), designation_atom.ptr<char>());
	LilvNode* rdfs_range = lilv_new_uri(
		world->lilv_world(), LILV_NS_RDFS "range");

	// Get every class in the range of the port's property
	RDFS::URISet ranges;
	LilvNodes* range = lilv_world_find_nodes(
		world->lilv_world(), designation, rdfs_range, NULL);
	LILV_FOREACH(nodes, r, range) {
		ranges.insert(Raul::URI(lilv_node_as_string(lilv_nodes_get(range, r))));
	}
	RDFS::classes(world, ranges, false);

	// Get all objects in range
	RDFS::Objects values = RDFS::instances(world, ranges);

	// Add a menu item for each such class
	for (const auto& v : values) {
		if (!v.second.empty()) {
			Glib::ustring label = world->rdf_world()->prefixes().qualify(v.first)
				+ " - " + v.second;
			menu->items().push_back(Gtk::Menu_Helpers::MenuElem(label));
			Gtk::MenuItem* menu_item = &(menu->items().back());
			menu_item->signal_activate().connect(
				sigc::bind(sigc::mem_fun(this, &Port::on_uri_activated),
				           v.first));
		}
	}

	return menu;
}

bool
Port::on_event(GdkEvent* ev)
{
	GraphBox* box = NULL;
	switch (ev->type) {
	case GDK_ENTER_NOTIFY:
		_entered = true;
		if ((box = get_graph_box())) {
			box->object_entered(model().get());
		}
		return false;
	case GDK_LEAVE_NOTIFY:
		_entered = false;
		if ((box = get_graph_box())) {
			box->object_left(model().get());
		}
		return false;
	case GDK_BUTTON_PRESS:
		if (ev->button.button == 1) {
			if (model()->is_enumeration()) {
				Gtk::Menu* menu = build_enum_menu();
				menu->popup(ev->button.button, ev->button.time);
				return true;
			} else if (model()->is_uri()) {
				Gtk::Menu* menu = build_uri_menu();
				if (menu) {
					menu->popup(ev->button.button, ev->button.time);
					return true;
				}
			}
		} else if (ev->button.button == 3) {
			return show_menu(&ev->button);
		}
		break;
	default:
		break;
	}

	return false;
}

/* Peak colour stuff */

static inline uint32_t
rgba_to_uint(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return ((((uint32_t)(r)) << 24) |
	        (((uint32_t)(g)) << 16) |
	        (((uint32_t)(b)) << 8) |
	        (((uint32_t)(a))));
}

static inline uint8_t
mono_interpolate(uint8_t v1, uint8_t v2, float f)
{
	return ((int)rint((v2) * (f) + (v1) * (1 - (f))));
}

#define RGBA_R(x) (((uint32_t)(x)) >> 24)
#define RGBA_G(x) ((((uint32_t)(x)) >> 16) & 0xFF)
#define RGBA_B(x) ((((uint32_t)(x)) >> 8) & 0xFF)
#define RGBA_A(x) (((uint32_t)(x)) & 0xFF)

static inline uint32_t
rgba_interpolate(uint32_t c1, uint32_t c2, float f)
{
	return rgba_to_uint(
		mono_interpolate(RGBA_R(c1), RGBA_R(c2), f),
		mono_interpolate(RGBA_G(c1), RGBA_G(c2), f),
		mono_interpolate(RGBA_B(c1), RGBA_B(c2), f),
		mono_interpolate(RGBA_A(c1), RGBA_A(c2), f));
}

inline static uint32_t
peak_color(float peak)
{
	static const uint32_t min      = 0x4A8A0EC0;
	static const uint32_t max      = 0xFFCE1FC0;
	static const uint32_t peak_min = 0xFF561FC0;
	static const uint32_t peak_max = 0xFF0A38C0;

	if (peak < 1.0) {
		return rgba_interpolate(min, max, peak);
	} else {
		return rgba_interpolate(peak_min, peak_max, fminf(peak, 2.0f) - 1.0f);
	}
}

/* End peak colour stuff */

void
Port::activity(const Atom& value)
{
	if (model()->is_a(_app.uris().lv2_AudioPort)) {
		set_fill_color(peak_color(value.get<float>()));
	} else if (_app.can_control(model().get()) && value.type() == _app.uris().atom_Float) {
		Ganv::Port::set_control_value(value.get<float>());
	} else {
		_app.port_activity(this);
	}
}

void
Port::disconnected_from(SPtr<PortModel> port)
{
	if (!model()->connected() && model()->is_a(_app.uris().lv2_AudioPort)) {
		set_fill_color(peak_color(0.0f));
	}
}

GraphBox*
Port::get_graph_box() const
{
	SPtr<const GraphModel> graph = dynamic_ptr_cast<const GraphModel>(model()->parent());
	if (!graph) {
		graph = dynamic_ptr_cast<const GraphModel>(model()->parent()->parent());
	}

	return _app.window_factory()->graph_box(graph);
}

void
Port::set_type_tag()
{
	const URIs& uris = _app.uris();
	std::string tag;
	if (model()->is_a(_app.uris().lv2_AudioPort)) {
		tag = "~";
	} else if (model()->is_a(_app.uris().lv2_CVPort)) {
		tag = "ℝ̰";
	} else if (model()->is_a(_app.uris().lv2_ControlPort)) {
		if (model()->is_enumeration()) {
			tag = "…";
		} else if (model()->is_integer()) {
			tag = "ℤ";
		} else if (model()->is_toggle()) {
			tag = ((model()->value() != _app.uris().forge.make(0.0f))
			       ? "☑" : "☐");

		} else {
			tag = "ℝ";
		}
	} else if (model()->is_a(_app.uris().atom_AtomPort)) {
		if (model()->supports(_app.uris().atom_Float)) {
			if (model()->is_toggle()) {
				tag = ((model()->value() != _app.uris().forge.make(0.0f))
				       ? "☑" : "☐");
			} else {
				tag = "ℝ";
			}
		}
		if (model()->supports(_app.uris().atom_Int)) {
			tag += "ℤ";
		}
		if (model()->supports(_app.uris().midi_MidiEvent)) {
			tag += "𝕄";
		}
		if (model()->supports(_app.uris().patch_Message)) {
			if (tag.empty()) {
				tag += "=";
			} else {
				tag += "̿";
			}
		}
		if (tag.empty()) {
			tag = "*";
		}

		if (model()->has_property(uris.atom_bufferType, uris.atom_Sequence)) {
			tag += "̤";
		}
	}

	if (!tag.empty()) {
		set_value_label(tag.c_str());
	}
}

void
Port::port_properties_changed()
{
	if (model()->is_toggle()) {
		set_control_is_toggle(true);
	} else if (model()->is_integer()) {
		set_control_is_integer(true);
	}
	set_type_tag();
}

void
Port::property_changed(const Raul::URI& key, const Atom& value)
{
	const URIs& uris = _app.uris();
	if (value.type() == uris.forge.Float) {
		float val = value.get<float>();
		if (key == uris.ingen_value && !get_grabbed()) {
			Ganv::Port::set_control_value(val);
			if (model()->is_toggle()) {
				std::string tag = (val == 0.0f) ? "☐" : "☑";
				if (model()->is_a(_app.uris().lv2_CVPort)) {
					tag += "̰";
				} else if (model()->has_property(uris.atom_bufferType,
				                                 uris.atom_Sequence)) {
					tag += "̤";
				}
				set_value_label(tag.c_str());
			}
		} else if (key == uris.lv2_minimum) {
			if (model()->port_property(uris.lv2_sampleRate)) {
				val *= _app.sample_rate();
			}
			set_control_min(val);
		} else if (key == uris.lv2_maximum) {
			if (model()->port_property(uris.lv2_sampleRate)) {
				val *= _app.sample_rate();
			}
			set_control_max(val);
		}
	} else if (key == uris.lv2_portProperty) {
		port_properties_changed();
	} else if (key == uris.lv2_name) {
		if (value.type() == uris.forge.String &&
		    _app.world()->conf().option("port-labels").get<int32_t>() &&
		    _app.world()->conf().option("human-names").get<int32_t>()) {
			set_label(value.ptr<char>());
		}
	}
}

void
Port::property_removed(const Raul::URI& key, const Atom& value)
{
	const URIs& uris = _app.uris();
	if (key == uris.lv2_minimum || key == uris.lv2_maximum) {
		update_metadata();
	}
}

bool
Port::on_selected(gboolean b)
{
	if (b) {
		SPtr<const PortModel> pm = _port_model.lock();
		if (pm) {
			SPtr<const BlockModel> block = dynamic_ptr_cast<const BlockModel>(pm->parent());
			GraphWindow* win = _app.window_factory()->parent_graph_window(block);
			if (win && win->documentation_is_visible() && block->plugin_model()) {
				bool html = false;
#ifdef HAVE_WEBKIT
				html = true;
#endif
				const std::string& doc = block->plugin_model()->port_documentation(
					pm->index(), html);
				win->set_documentation(doc, html);
			}
		}
	}

	return true;
}

} // namespace GUI
} // namespace Ingen
