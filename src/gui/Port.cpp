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
#include "raul/log.hpp"
#include "flowcanvas/Module.hpp"
#include "interface/EngineInterface.hpp"
#include "shared/LV2URIMap.hpp"
#include "client/PatchModel.hpp"
#include "client/PortModel.hpp"
#include "App.hpp"
#include "Configuration.hpp"
#include "GladeFactory.hpp"
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

ArtVpathDash* Port::_dash;


SharedPtr<Port>
Port::create(
		boost::shared_ptr<FlowCanvas::Module> module,
		SharedPtr<PortModel>                  pm,
		bool                                  human_name,
		bool                                  flip)
{
	Glib::ustring label(human_name ? "" : pm->path().symbol());
	if (human_name) {
		const Raul::Atom& name = pm->get_property(App::instance().uris().lv2_name);
		if (name.type() == Raul::Atom::STRING) {
			label = name.get_string();
		} else {
			const SharedPtr<const NodeModel> parent(PtrCast<const NodeModel>(pm->parent()));
			if (parent && parent->plugin_model())
				label = parent->plugin_model()->port_human_name(pm->index());
		}
	}
	return SharedPtr<Port>(new Port(module, pm, label, flip));
}


/** @a flip Make an input port appear as an output port, and vice versa.
 */
Port::Port(
		boost::shared_ptr<FlowCanvas::Module> module,
		SharedPtr<PortModel>                  pm,
		const string&                         name,
		bool                                  flip)
	: FlowCanvas::Port(module, name,
			flip ? (!pm->is_input()) : pm->is_input(),
			App::instance().configuration()->get_port_color(pm.get()))
	, _port_model(pm)
	, _pressed(false)
	, _flipped(flip)
{
	assert(module);
	assert(pm);

	delete _menu;
	_menu = NULL;

	ArtVpathDash* dash = this->dash();
	_rect->property_dash() = dash;
	set_border_width(dash ? 2.0 : 0.0);

	pm->signal_moved.connect(sigc::mem_fun(this, &Port::moved));

	if (pm->type().is_control()) {
		set_toggled(pm->is_toggle());
		show_control();
		pm->signal_property.connect(sigc::mem_fun(this, &Port::property_changed));
		pm->signal_value_changed.connect(sigc::mem_fun(this, &Port::value_changed));
	}

	pm->signal_activity.connect(sigc::mem_fun(this, &Port::activity));

	update_metadata();

	value_changed(pm->value());
}


Port::~Port()
{
	App::instance().activity_port_destroyed(this);
}


void
Port::update_metadata()
{
	SharedPtr<PortModel> pm = _port_model.lock();
	if (pm && pm->type().is_control()) {
		boost::shared_ptr<NodeModel> parent = PtrCast<NodeModel>(pm->parent());
		if (parent) {
			float min = 0.0f;
			float max = 1.0f;
			parent->port_value_range(pm, min, max);
			set_control_min(min);
			set_control_max(max);
		}
	}
}


void
Port::create_menu()
{
	PortMenu* menu = NULL;
	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference();
	xml->get_widget_derived("object_menu", menu);
	menu->init(model(), _flipped);
	set_menu(menu);
}


void
Port::moved()
{
	if (App::instance().configuration()->name_style() == Configuration::PATH)
		set_name(model()->symbol().c_str());
	module().lock()->resize();
}


void
Port::value_changed(const Atom& value)
{
	if (_pressed)
		return;
	else if (value.type() == Atom::FLOAT)
		FlowCanvas::Port::set_control(value.get_float());
	else
		warn << "Unknown port value type " << (unsigned)value.type() << endl;
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

	return false;
}


void
Port::activity()
{
	App::instance().port_activity(this);
}


void
Port::set_control(float value, bool signal)
{
	if (signal) {
		App&                        app   = App::instance();
		Ingen::Shared::World* const world = app.world();
		if (model()->type() == PortType::CONTROL) {
			app.engine()->set_property(model()->path(),
					world->uris->ingen_value, Atom(value));
			PatchWindow* pw = app.window_factory()->patch_window(
					PtrCast<PatchModel>(model()->parent()));
			if (!pw)
				pw = app.window_factory()->patch_window(
						PtrCast<PatchModel>(model()->parent()->parent()));
			if (pw)
				pw->show_port_status(model().get(), value);

		} else if (model()->type() == PortType::EVENTS) {
			app.engine()->set_property(model()->path(),
					world->uris->ingen_value,
					Atom("http://example.org/ev#BangEvent", 0, NULL));
		}
	}

	FlowCanvas::Port::set_control(value);
}


void
Port::property_changed(const URI& key, const Atom& value)
{
	const LV2URIMap& uris = App::instance().uris();
	if (value.type() == Atom::FLOAT) {
		if (key == uris.ingen_value && !_pressed)
			set_control(value.get_float(), false);
		else if (key == uris.lv2_minimum)
			set_control_min(value.get_float());
		else if (key == uris.lv2_maximum)
			set_control_max(value.get_float());
	} else if (value.type() == Atom::BOOL) {
		if ((key == uris.lv2_toggled))
			set_toggled(value.get_bool());
	} else if (value.type() == Atom::URI) {
		ArtVpathDash* dash = this->dash();
		_rect->property_dash() = dash;
		set_border_width(dash ? 2.0 : 0.0);
	}
}


ArtVpathDash*
Port::dash()
{
	const LV2URIMap& uris = App::instance().uris();
	SharedPtr<PortModel> pm = _port_model.lock();
	if (!pm)
		return NULL;

	const Raul::Atom& context = pm->get_property(uris.ctx_context);
	if (!context.is_valid() || context.type() != Atom::URI || context == uris.ctx_AudioContext)
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


} // namespace GUI
} // namespace Ingen
