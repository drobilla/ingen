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
#include "interface/EngineInterface.hpp"
#include "flowcanvas/Module.hpp"
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
	set_name(model()->path().name());
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
		_pressed = true;
		break;
	case GDK_BUTTON_RELEASE:
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
		if (model()->type() == PortType::CONTROL) {
			App::instance().engine()->set_port_value(model()->path(), Atom(value));
			PatchWindow* pw = App::instance().window_factory()->patch_window(
					PtrCast<PatchModel>(model()->parent()));
			if (!pw)
				pw = App::instance().window_factory()->patch_window(
						PtrCast<PatchModel>(model()->parent()->parent()));
			if (pw)
				pw->show_port_status(model().get(), value);

		} else if (model()->type() == PortType::EVENTS) {
			App::instance().engine()->set_port_value(model()->path(),
					Atom("<http://example.org/ev#BangEvent>", 0, NULL));
		}
	}

	FlowCanvas::Port::set_control(value);
}


void
Port::property_changed(const URI& key, const Atom& value)
{
	if (value.type() == Atom::FLOAT) {
		if (key.str() == "ingen:value")
			set_control(value.get_float(), false);
		else if (key.str() == "lv2:minimum")
			set_control_min(value.get_float());
		else if (key.str() == "lv2:maximum")
			set_control_max(value.get_float());
	} else if (value.type() == Atom::BOOL) {
		if ((key.str() == "lv2:toggled"))
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
	SharedPtr<PortModel> pm = _port_model.lock();
	if (!pm)
		return NULL;

	const Raul::Atom& context = pm->get_property("ctx:context");
	if (!context.is_valid() || context.type() != Atom::URI
			|| !strcmp(context.get_uri(), "ctx:AudioContext"))
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
