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
#include "Controls.hpp"
#include "ControlPanel.hpp"
#include "PortPropertiesWindow.hpp"
#include "GladeFactory.hpp"
#include "App.hpp"

using namespace std;
using namespace Ingen::Client;

namespace Ingen {
namespace GUI {


// ////////////////////// Control ///////////////////////////////// //

Control::Control(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml)
	: Gtk::VBox(cobject)
	, _control_panel(NULL)
	, _enable_signal(false)
{
	Glib::RefPtr<Gnome::Glade::Xml> menu_xml = GladeFactory::new_glade_reference("port_control_menu");
	menu_xml->get_widget("port_control_menu", _menu);
	menu_xml->get_widget("port_control_menu_properties", _menu_properties);
	
	_menu_properties->signal_activate().connect(
		sigc::mem_fun(this, &SliderControl::menu_properties));
}
	

Control::~Control()
{
	_enable_signal = false;
	_control_connection.disconnect();
	_port_model.reset();
}


void
Control::init(ControlPanel* panel, SharedPtr<PortModel> pm)
{
	_control_panel = panel;
	_port_model = pm,

	assert(_port_model);
	assert(panel);

	_control_connection = pm->signal_value_changed.connect(sigc::mem_fun(this, &Control::set_value));
}

	
void
Control::menu_properties()
{
	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference();

	PortPropertiesWindow* window;
	xml->get_widget_derived("port_properties_win", window);
	window->present(_port_model);
}


// ////////////////// SliderControl ////////////////////// //


SliderControl::SliderControl(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Control(cobject, xml)
	, _enabled(true)
{
	xml->get_widget("control_strip_name_label", _name_label);
	xml->get_widget("control_strip_slider", _slider);
	xml->get_widget("control_strip_spinner", _value_spinner);
}


void
SliderControl::init(ControlPanel* panel, SharedPtr<PortModel> pm)
{
	_enable_signal = false;
	_enabled = true;

	Control::init(panel, pm);

	assert(_name_label);
	assert(_slider);

	set_name(pm->path().name());
	
	_slider->set_draw_value(false);

	signal_button_press_event().connect(sigc::mem_fun(*this, &SliderControl::clicked));
	_slider->signal_button_press_event().connect(sigc::mem_fun(*this, &SliderControl::clicked));

	_slider->signal_event().connect(
			sigc::mem_fun(*this, &SliderControl::slider_pressed));

	_slider->signal_value_changed().connect(
			sigc::mem_fun(*this, &SliderControl::update_value_from_slider));
	
	_value_spinner->signal_value_changed().connect(
			sigc::mem_fun(*this, &SliderControl::update_value_from_spinner));

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
	
	pm->signal_variable.connect(sigc::mem_fun(this, &SliderControl::port_variable_change));

	_slider->set_range(std::min(min, pm->value().get_float()), std::max(max, pm->value().get_float()));
	//_value_spinner->set_range(min, max);

	set_value(pm->value());

	_enable_signal = true;

	show_all();
}


bool
SliderControl::clicked(GdkEventButton* ev)
{
	if (ev->button == 3) {
		_menu->popup(ev->button, ev->time);
		return true;
	} else {
		return false;
	}
}


void
SliderControl::set_value(const Atom& atom)
{
	float val = atom.get_float();
	
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
SliderControl::port_variable_change(const string& key, const Atom& value)
{
	if ( (key == "ingen:minimum") && value.type() == Atom::FLOAT)
		set_range(value.get_float(), _slider->get_adjustment()->get_upper());
	else if ( (key == "ingen:maximum") && value.type() == Atom::FLOAT)
		set_range(_slider->get_adjustment()->get_lower(), value.get_float());
}


void
SliderControl::set_range(float min, float max)
{
	_slider->set_range(min, max);
	//_value_spinner->set_range(min, max);
}


void
SliderControl::set_name(const string& name)
{
	string name_label = "<span weight=\"bold\">";
	name_label += name + "</span>";
	_name_label->set_markup(name_label);
}


void
SliderControl::enable()
{
	_slider->property_sensitive() = true;
	//_min_spinner->property_sensitive() = true;
	//_max_spinner->property_sensitive() = true;
	_value_spinner->property_sensitive() = true;
	_name_label->property_sensitive() = true;
}


void
SliderControl::disable()
{
	_slider->property_sensitive() = false;
	//_min_spinner->property_sensitive() = false;
	//_max_spinner->property_sensitive() = false;
	_value_spinner->property_sensitive() = false;
	_name_label->property_sensitive() = false;
}


void
SliderControl::update_value_from_slider()
{
	if (_enable_signal) {
		float value = _slider->get_value();
		bool change = true;
			
		_enable_signal = false;
		
		if (_port_model->is_integer()) {
			value = lrintf(value);
			if (value == lrintf(_port_model->value().get_float()))
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
SliderControl::update_value_from_spinner()
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
SliderControl::slider_pressed(GdkEvent* ev)
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


// ///////////// IntegerControl ////////////// //

#if 0
IntegerControl::IntegerControl(ControlPanel* panel, SharedPtr<PortModel> pm)
: Control(panel, pm),
  _enable_signal(false),
  _alignment(0.5, 0.5, 0.0, 0.0),
  _name_label(pm->path().name()),
  _spinner(1.0, 0)
{
	set_name(pm->path().name());

	_spinner.set_range(-99999, 99999);
	_spinner.set_value(_port_model->value());
	_spinner.signal_value_changed().connect(
		sigc::mem_fun(*this, &IntegerControl::update_value));
	_spinner.set_increments(1, 10);
	
	_alignment.add(_spinner);
	pack_start(_name_label);
	pack_start(_alignment);

	_enable_signal = true;

	show_all();
}


void
IntegerControl::set_name(const string& name)
{
	string name_label = "<span weight=\"bold\">";
	name_label += name + "</span>";
	_name_label->set_markup(name_label);
}


void
IntegerControl::set_value(float val)
{
	//cerr << "[IntegerControl] Setting value to " << val << endl;
	_enable_signal = false;
	_spinner.set_value(val);
	_enable_signal = true;
}


void
IntegerControl::enable()
{
	_spinner.property_sensitive() = true;
	_name_label->property_sensitive() = true;
}


void
IntegerControl::disable()
{
	_spinner.property_sensitive() = false;
	_name_label->property_sensitive() = false;
}


void
IntegerControl::update_value()
{
	if (_enable_signal) {
		float value = _spinner.get_value();
		_control_panel->value_changed(_port_model, value);
		//m_port_model->value(value);
	}
}
#endif


// ///////////// ToggleControl ////////////// //


ToggleControl::ToggleControl(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Control(cobject, xml)
{
	xml->get_widget("toggle_control_name_label", _name_label);
	xml->get_widget("toggle_control_check", _checkbutton);
}

	
void
ToggleControl::init(ControlPanel* panel, SharedPtr<PortModel> pm)
{
	_enable_signal = false;

	Control::init(panel, pm);

	assert(_name_label);
	assert(_checkbutton);

	set_name(pm->path().name());

	_checkbutton->signal_toggled().connect(sigc::mem_fun(*this, &ToggleControl::toggled));
	set_value(pm->value());

	_enable_signal = true;
	show_all();
}


void
ToggleControl::set_name(const string& name)
{
	string name_label = "<span weight=\"bold\">";
	name_label += name + "</span>";
	_name_label->set_markup(name_label);
}


void
ToggleControl::set_value(const Atom& val)
{
	bool enable = false;
	switch (val.type()) {
		case Atom::FLOAT:
			enable = (val.get_float() != 0.0f);
			break;
		case Atom::INT:
			enable = (val.get_int32() != 0);
			break;
		case Atom::BOOL:
			enable = (val.get_bool());
		default:
			cerr << "Unsupported value type for toggle control" << endl;
	}
	
	_enable_signal = false;
	_checkbutton->set_active(enable);
	_enable_signal = true;
}


void
ToggleControl::enable()
{
	_checkbutton->property_sensitive() = true;
	_name_label->property_sensitive() = true;
}


void
ToggleControl::disable()
{
	_checkbutton->property_sensitive() = false;
	_name_label->property_sensitive() = false;
}


void
ToggleControl::toggled()
{
	if (_enable_signal) {
		float value = _checkbutton->get_active() ? 1.0f : 0.0f;
		_control_panel->value_changed(_port_model, value);
		//m_port_model->value(value);
	}
}


} // namespace GUI
} // namespace Ingen
