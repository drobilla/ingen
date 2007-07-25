/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
#include <string>
#include "interface/EngineInterface.hpp"
#include "client/NodeModel.hpp"
#include "client/PluginModel.hpp"
#include "App.hpp"
#include "ControlGroups.hpp"
#include "PortPropertiesWindow.hpp"

using std::string;

namespace Ingen {
namespace GUI {


PortPropertiesWindow::PortPropertiesWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Gtk::Dialog(cobject)
	, _enable_signal(false)
	, _control(NULL)
{
	xml->get_widget("port_properties_min_spinner", _min_spinner);
	xml->get_widget("port_properties_max_spinner", _max_spinner);
	xml->get_widget("port_properties_cancel_button", _cancel_button);
	xml->get_widget("port_properties_ok_button", _ok_button);
	
	_cancel_button->signal_clicked().connect(sigc::mem_fun(this,
				&PortPropertiesWindow::cancel));
	
	_ok_button->signal_clicked().connect(sigc::mem_fun(this,
				&PortPropertiesWindow::ok));
}


/** Set the port this window is associated with.
 * This function MUST be called before using this object in any way.
 */
void
PortPropertiesWindow::init(ControlGroup* control, SharedPtr<PortModel> pm)
{
	assert(pm);
	assert(control);
	
	_port_model = pm;
	_control = control;


	set_title(pm->path() + " Properties");
	
	// FIXME: code duplication w/ ControlGroups.cpp
	float min = 0.0f;
	float max = 1.0f;

	const Atom& min_atom = pm->get_metadata("ingen:minimum");
	const Atom& max_atom = pm->get_metadata("ingen:maximum");
	if (min_atom.type() == Atom::FLOAT && max_atom.type() == Atom::FLOAT) {
		min = min_atom.get_float();
		max = max_atom.get_float();
	}
	
	const SharedPtr<NodeModel> parent = PtrCast<NodeModel>(pm->parent());

#ifdef HAVE_SLV2
	if (parent && parent->plugin() && parent->plugin()->type() == PluginModel::LV2) {
		min = slv2_port_get_minimum_value(
				parent->plugin()->slv2_plugin(),
				slv2_plugin_get_port_by_symbol(parent->plugin()->slv2_plugin(),
					pm->path().name().c_str()));
		max = slv2_port_get_maximum_value(
					parent->plugin()->slv2_plugin(),
					slv2_plugin_get_port_by_symbol(parent->plugin()->slv2_plugin(),
						pm->path().name().c_str()));
	}
#endif

	if (max <= min)
		max = min + 1.0f;

	_initial_min = min;
	_initial_max = max;

	_min_spinner->set_value(min);
	_min_spinner->signal_value_changed().connect(sigc::mem_fun(*this, &PortPropertiesWindow::min_changed));
	_max_spinner->set_value(max);
	_max_spinner->signal_value_changed().connect(sigc::mem_fun(*this, &PortPropertiesWindow::max_changed));
	
	pm->metadata_update_sig.connect(sigc::mem_fun(this, &PortPropertiesWindow::metadata_update));

	_enable_signal = true;
}


void
PortPropertiesWindow::metadata_update(const string& key, const Atom& value)
{
	_enable_signal = false;

	if ( (key == "ingen:minimum") && value.type() == Atom::FLOAT)
		_min_spinner->set_value(value.get_float());
	else if ( (key == "ingen:maximum") && value.type() == Atom::FLOAT)
		_max_spinner->set_value(value.get_float());
	
	_enable_signal = true;
}


void
PortPropertiesWindow::min_changed()
{
	const float min = _min_spinner->get_value();
	float max       = _max_spinner->get_value();
	
	if (max <= min) {
		max = min + 1.0;
		_max_spinner->set_value(max);
	}

	_control->set_range(min, max);

	if (_enable_signal)
		App::instance().engine()->set_metadata(_port_model->path(), "ingen:minimum", min);
}


void
PortPropertiesWindow::max_changed()
{
	float min       = _min_spinner->get_value();
	const float max = _max_spinner->get_value();
	
	if (min >= max) {
		min = max - 1.0;
		_min_spinner->set_value(min);
	}

	_control->set_range(min, max);

	if (_enable_signal)
		App::instance().engine()->set_metadata(_port_model->path(), "ingen:maximum", max);
}


void
PortPropertiesWindow::cancel()
{
	App::instance().engine()->set_metadata(_port_model->path(), "ingen:minimum", _initial_min);
	App::instance().engine()->set_metadata(_port_model->path(), "ingen:maximum", _initial_max);
	delete this;
}


void
PortPropertiesWindow::ok()
{
	delete this;
}


} // namespace GUI
} // namespace Ingen

