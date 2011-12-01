/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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
#include "raul/log.hpp"
#include "flowcanvas/Module.hpp"
#include "ingen/ServerInterface.hpp"
#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/client/PatchModel.hpp"
#include "ingen/client/PortModel.hpp"
#include "App.hpp"
#include "Configuration.hpp"
#include "WidgetFactory.hpp"
#include "PatchWindow.hpp"
#include "Port.hpp"
#include "PortMenu.hpp"
#include "WindowFactory.hpp"

using namespace Ingen::Client;
using namespace std;
using namespace Raul;

namespace Ingen {
using namespace Shared;
namespace GUI {

//ArtVpathDash* Port::_dash;

Port*
Port::create(App&                       app,
             FlowCanvas::Module&        module,
             SharedPtr<const PortModel> pm,
             bool                       human_name,
             bool                       flip)
{
	Glib::ustring label(human_name ? "" : pm->path().symbol());
	if (human_name) {
		const Raul::Atom& name = pm->get_property(app.uris().lv2_name);
		if (name.type() == Raul::Atom::STRING) {
			label = name.get_string();
		} else {
			const SharedPtr<const NodeModel> parent(PtrCast<const NodeModel>(pm->parent()));
			if (parent && parent->plugin_model())
				label = parent->plugin_model()->port_human_name(pm->index());
		}
	}
	return new Port(app, module, pm, label, flip);
}

/** @a flip Make an input port appear as an output port, and vice versa.
 */
Port::Port(App&                       app,
           FlowCanvas::Module&        module,
           SharedPtr<const PortModel> pm,
           const string&              name,
           bool                       flip)
	: FlowCanvas::Port(module, name,
			flip ? (!pm->is_input()) : pm->is_input(),
			app.configuration()->get_port_color(pm.get()))
	, _app(app)
	, _port_model(pm)
	, _pressed(false)
	, _flipped(flip)
{
	assert(pm);

	//ArtVpathDash* dash = this->dash();
	//_rect.property_dash() = dash;
	set_border_width(1.0);

	pm->signal_moved().connect(sigc::mem_fun(this, &Port::moved));

	if (app.can_control(pm.get())) {
		set_toggled(pm->is_toggle());
		show_control();
		pm->signal_property().connect(
			sigc::mem_fun(this, &Port::property_changed));
		pm->signal_value_changed().connect(
			sigc::mem_fun(this, &Port::value_changed));
	}

	pm->signal_activity().connect(sigc::mem_fun(this, &Port::activity));

	update_metadata();

	value_changed(pm->value());
}

Port::~Port()
{
	_app.activity_port_destroyed(this);
}

void
Port::update_metadata()
{
	SharedPtr<const PortModel> pm = _port_model.lock();
	if (_app.can_control(pm.get()) && pm->is_numeric()) {
		boost::shared_ptr<const NodeModel> parent = PtrCast<const NodeModel>(pm->parent());
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
	menu->init(_app, model(), _flipped);
	menu->popup(ev->button, ev->time);
	return true;
}

void
Port::moved()
{
	if (_app.configuration()->name_style() == Configuration::PATH)
		set_name(model()->symbol().c_str());
}

void
Port::value_changed(const Atom& value)
{
	if (_pressed)
		return;
	else if (value.type() == Atom::FLOAT)
		FlowCanvas::Port::set_control(value.get_float());
}

bool
Port::on_event(GdkEvent* ev)
{
	switch (ev->type) {
	case GDK_BUTTON_PRESS:
		if (ev->button.button == 1)
			_pressed = true;
		break;
	case GDK_BUTTON_RELEASE:
		if (ev->button.button == 1)
			_pressed = false;
	default:
		break;
	}

	return Object::on_event(ev);
}

bool
Port::on_click(GdkEventButton* ev)
{
	if (ev->button == 3) {
		return show_menu(ev);
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
Port::activity(const Raul::Atom& value)
{
	if (model()->is_a(_app.uris().lv2_AudioPort)) {
		set_fill_color(peak_color(value.get_float()));
	} else {
		_app.port_activity(this);
	}
}

void
Port::set_control(float value, bool signal)
{
	if (signal) {
		Ingen::Shared::World* const world = _app.world();
		_app.engine()->set_property(model()->path(),
				world->uris()->ingen_value, Atom(value));
		PatchWindow* pw = _app.window_factory()->patch_window(
				PtrCast<const PatchModel>(model()->parent()));
		if (!pw)
			pw = _app.window_factory()->patch_window(
					PtrCast<const PatchModel>(model()->parent()->parent()));
		if (pw)
			pw->show_port_status(model().get(), value);
	}

	FlowCanvas::Port::set_control(value);
}

void
Port::property_changed(const URI& key, const Atom& value)
{
	const URIs& uris = _app.uris();
	if (value.type() == Atom::FLOAT) {
		float val = value.get_float();
		if (key == uris.ingen_value && !_pressed) {
			set_control(val, false);
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
		if (value == uris.lv2_toggled)
			set_toggled(true);
	} else if (key == uris.ctx_context) {
		Raul::info << "TODO: Visual indication of port context?" << std::endl;
	} else if (key == uris.lv2_name) {
		if (value.type() == Atom::STRING
				&& _app.configuration()->name_style() == Configuration::HUMAN) {
			set_name(value.get_string());
		}
	}
}

/*
ArtVpathDash*
Port::dash()
{
	const URIs& uris = _app.uris();
	SharedPtr<const PortModel> pm = _port_model.lock();
	if (!pm)
		return NULL;

	if (pm->has_context(uris.ctx_audioContext))
		return NULL;

	if (!_dash) {
		_dash = new ArtVpathDash();
		_dash->n_dash = 2;
		_dash->dash = art_new(double, 2);
		_dash->dash[0] = 4;
		_dash->dash[1] = 4;
	}

	return _dash;
}
*/

void
Port::set_selected(bool b)
{
	if (b != selected()) {
		FlowCanvas::Port::set_selected(b);
		SharedPtr<const PortModel> pm = _port_model.lock();
		if (pm && b) {
			SharedPtr<const NodeModel> node = PtrCast<NodeModel>(pm->parent());
			PatchWindow*               win  = _app.window_factory()->parent_patch_window(node);
			if (win && node->plugin_model()) {
				const std::string& doc = node->plugin_model()->port_documentation(
					pm->index());
				if (!doc.empty()) {
					win->show_documentation(doc, false);
				} else {
					win->hide_documentation();
				}
			}
		}
	}
}

} // namespace GUI
} // namespace Ingen
