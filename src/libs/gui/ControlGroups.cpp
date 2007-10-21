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

#include CONFIG_H_PATH

#include <cmath>
#include <iostream>
#include <algorithm>
#include "interface/EngineInterface.hpp"
#include "client/PluginModel.hpp"
#include "client/NodeModel.hpp"
#include "client/PortModel.hpp"
#include "ControlGroups.hpp"
#include "ControlPanel.hpp"
#include "PortPropertiesWindow.hpp"
#include "GladeFactory.hpp"
#include "App.hpp"

using namespace std;
using namespace Ingen::Client;

namespace Ingen {
namespace GUI {


// ////////////////////// ControlGroup ///////////////////////////////// //

ControlGroup::ControlGroup(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml)
: Gtk::VBox(cobject),
  _control_panel(NULL),
  _enable_signal(false)
{
}
	

ControlGroup::~ControlGroup()
{
	_enable_signal = false;
	_control_connection.disconnect();
	_port_model.reset();
}


void
ControlGroup::init(ControlPanel* panel, SharedPtr<PortModel> pm)
{
	_control_panel = panel;
	_port_model = pm,

	assert(_port_model);
	assert(panel);

	_control_connection = pm->signal_control.connect(sigc::mem_fun(this, &ControlGroup::set_value));
}


// ////////////////// SliderControlGroup ////////////////////// //


SliderControlGroup::SliderControlGroup(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: ControlGroup(cobject, xml),
  _enabled(true)
{
	xml->get_widget("control_strip_name_label", _name_label);
	xml->get_widget("control_strip_slider", _slider);
	xml->get_widget("control_strip_spinner", _value_spinner);
	
	Glib::RefPtr<Gnome::Glade::Xml> menu_xml = GladeFactory::new_glade_reference("port_control_menu");
	menu_xml->get_widget("port_control_menu", _menu);
	menu_xml->get_widget("port_control_menu_properties", _menu_properties);
	
	_menu_properties->signal_activate().connect(
		sigc::mem_fun(this, &SliderControlGroup::menu_properties));
}


void
SliderControlGroup::init(ControlPanel* panel, SharedPtr<PortModel> pm)
{
	_enable_signal = false;
	_enabled = true;

	ControlGroup::init(panel, pm);

	assert(_name_label);
	assert(_slider);

	set_name(pm->path().name());
	
	_slider->set_draw_value(false);

	signal_button_press_event().connect(sigc::mem_fun(*this, &SliderControlGroup::clicked));
	_slider->signal_button_press_event().connect(sigc::mem_fun(*this, &SliderControlGroup::clicked));

	_slider->signal_event().connect(
			sigc::mem_fun(*this, &SliderControlGroup::slider_pressed));

	_slider->signal_value_changed().connect(
			sigc::mem_fun(*this, &SliderControlGroup::update_value_from_slider));
	
	_value_spinner->signal_value_changed().connect(
			sigc::mem_fun(*this, &SliderControlGroup::update_value_from_spinner));

	float min = 0.0f, max = 1.0f;
	
	boost::shared_ptr<NodeModel> parent = PtrCast<NodeModel>(_port_model->parent());
	if (parent)
		parent->port_value_range(_port_model, min, max);

	if (pm->is_integer() || pm->is_toggle()) {
		_slider->set_increments(1, 10);
		_value_spinner->set_digits(0);
	} else {
		_slider->set_increments(0, 0);
	}
	
	pm->signal_variable.connect(sigc::mem_fun(this, &SliderControlGroup::port_variable_change));

	_slider->set_range(std::min(min, pm->value()), std::max(max, pm->value()));
	//_value_spinner->set_range(min, max);

	set_value(pm->value());

	_enable_signal = true;

	show_all();
}


bool
SliderControlGroup::clicked(GdkEventButton* ev)
{
	if (ev->button == 3) {
		_menu->popup(ev->button, ev->time);
		return true;
	} else {
		return false;
	}
}


void
SliderControlGroup::menu_properties()
{
	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference();

	PortPropertiesWindow* window;
	xml->get_widget_derived("port_properties_win", window);
	window->present(_port_model);
}


void
SliderControlGroup::set_value(float val)
{
	if (_port_model->is_integer())
		val = lrintf(val);

	_enable_signal = false;
	if (_enabled) {
		if (_slider->get_value() != val) {
			const Gtk::Adjustment* range = _slider->get_adjustment();
			const float lower = range->get_lower();
			const float upper = range->get_upper();
			if (val < lower || val > upper)
				set_range(min(lower, val), max(lower, val));
			_slider->set_value(val);
		}
		if (_value_spinner->get_value() != val)
			_value_spinner->set_value(val);
	}
	_enable_signal = true;
}

	
void
SliderControlGroup::port_variable_change(const string& key, const Atom& value)
{
	if ( (key == "ingen:minimum") && value.type() == Atom::FLOAT)
		set_range(value.get_float(), _slider->get_adjustment()->get_upper());
	else if ( (key == "ingen:maximum") && value.type() == Atom::FLOAT)
		set_range(_slider->get_adjustment()->get_lower(), value.get_float());
}


void
SliderControlGroup::set_range(float min, float max)
{
	_slider->set_range(min, max);
	//_value_spinner->set_range(min, max);
}


void
SliderControlGroup::set_name(const string& name)
{
	string name_label = "<span weight=\"bold\">";
	name_label += name + "</span>";
	_name_label->set_markup(name_label);
}


void
SliderControlGroup::enable()
{
	_slider->property_sensitive() = true;
	//_min_spinner->property_sensitive() = true;
	//_max_spinner->property_sensitive() = true;
	_value_spinner->property_sensitive() = true;
	_name_label->property_sensitive() = true;
}


void
SliderControlGroup::disable()
{
	_slider->property_sensitive() = false;
	//_min_spinner->property_sensitive() = false;
	//_max_spinner->property_sensitive() = false;
	_value_spinner->property_sensitive() = false;
	_name_label->property_sensitive() = false;
}


void
SliderControlGroup::update_value_from_slider()
{
	if (_enable_signal) {
		float value = _slider->get_value();
		bool change = true;
			
		_enable_signal = false;
		
		if (_port_model->is_integer()) {
			value = lrintf(value);
			if (value == lrintf(_port_model->value()))
				change = false;
		}
		
		if (change) {
			_value_spinner->set_value(value);
			_control_panel->value_changed(_port_model, value);
		}
		
		_enable_signal = true;
	}
}


void
SliderControlGroup::update_value_from_spinner()
{
	if (_enable_signal) {
		_enable_signal = false;
		const float value = _value_spinner->get_value();

		set_value(value);

		_control_panel->value_changed(_port_model, value);
		
		//m_port_model->value(value);
		_enable_signal = true;
	}
}


/** Callback for when slider is grabbed so we can ignore set_control
 * events for this port (and avoid nasty feedback issues).
 */
bool
SliderControlGroup::slider_pressed(GdkEvent* ev)
{
	//cerr << "Pressed: " << ev->type << endl;
	if (ev->type == GDK_BUTTON_PRESS) {
		_enabled = false;
		//GtkClientInterface::instance()->set_ignore_port(_port_model->path());
	} else if (ev->type == GDK_BUTTON_RELEASE) {
		_enabled = true;
		//GtkClientInterface::instance()->clear_ignore_port();
	}
	
	return false;
}


// ///////////// IntegerControlGroup ////////////// //

#if 0
IntegerControlGroup::IntegerControlGroup(ControlPanel* panel, SharedPtr<PortModel> pm)
: ControlGroup(panel, pm),
  _enable_signal(false),
  _alignment(0.5, 0.5, 0.0, 0.0),
  _name_label(pm->path().name()),
  _spinner(1.0, 0)
{
	set_name(pm->path().name());

	_spinner.set_range(-99999, 99999);
	_spinner.set_value(_port_model->value());
	_spinner.signal_value_changed().connect(
		sigc::mem_fun(*this, &IntegerControlGroup::update_value));
	_spinner.set_increments(1, 10);
	
	_alignment.add(_spinner);
	pack_start(_name_label);
	pack_start(_alignment);

	_enable_signal = true;

	show_all();
}


void
IntegerControlGroup::set_name(const string& name)
{
	string name_label = "<span weight=\"bold\">";
	name_label += name + "</span>";
	_name_label->set_markup(name_label);
}


void
IntegerControlGroup::set_value(float val)
{
	//cerr << "[IntegerControlGroup] Setting value to " << val << endl;
	_enable_signal = false;
	_spinner.set_value(val);
	_enable_signal = true;
}


void
IntegerControlGroup::enable()
{
	_spinner.property_sensitive() = true;
	_name_label->property_sensitive() = true;
}


void
IntegerControlGroup::disable()
{
	_spinner.property_sensitive() = false;
	_name_label->property_sensitive() = false;
}


void
IntegerControlGroup::update_value()
{
	if (_enable_signal) {
		float value = _spinner.get_value();
		_control_panel->value_changed(_port_model, value);
		//m_port_model->value(value);
	}
}


// ///////////// ToggleControlGroup ////////////// //


ToggleControlGroup::ToggleControlGroup(ControlPanel* panel, SharedPtr<PortModel> pm)
: ControlGroup(panel, pm),
  _enable_signal(false),
  _alignment(0.5, 0.5, 0.0, 0.0),
  _name_label(pm->path().name())
{
	set_name(pm->path().name());

	set_value(_port_model->value());
	_checkbutton.signal_toggled().connect(
		sigc::mem_fun(*this, &ToggleControlGroup::update_value));
	
	_alignment.add(_checkbutton);
	pack_start(_name_label);
	pack_start(_alignment);

	_enable_signal = true;

	show_all();
}


void
ToggleControlGroup::set_name(const string& name)
{
	string name_label = "<span weight=\"bold\">";
	name_label += name + "</span>";
	_name_label->set_markup(name_label);
}


void
ToggleControlGroup::set_value(float val)
{
	//cerr << "[ToggleControlGroup] Setting value to " << val << endl;
	_enable_signal = false;
	_checkbutton.set_active( (val > 0.0f) );
	_enable_signal = true;
}


void
ToggleControlGroup::enable()
{
	_checkbutton.property_sensitive() = true;
	_name_label->property_sensitive() = true;
}


void
ToggleControlGroup::disable()
{
	_checkbutton.property_sensitive() = false;
	_name_label->property_sensitive() = false;
}


void
ToggleControlGroup::update_value()
{
	if (_enable_signal) {
		float value = _checkbutton.get_active() ? 1.0f : 0.0f;
		_control_panel->value_changed(_port_model, value);
		//m_port_model->value(value);
	}
}
#endif

} // namespace GUI
} // namespace Ingen
