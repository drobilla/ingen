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

#include <cmath>
#include <iostream>
#include "ModelEngineInterface.h"
#include "ControlGroups.h"
#include "ControlPanel.h"
#include "PluginModel.h"
#include "NodeModel.h"
#include "PortModel.h"
#include "App.h"

using std::cerr; using std::cout; using std::endl;

using namespace Ingen::Client;

namespace Ingenuity {


// ////////////////////// ControlGroup ///////////////////////////////// //

ControlGroup::ControlGroup(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml)
: Gtk::VBox(cobject),
  _control_panel(NULL),
  _has_separator(false),
  _separator(NULL),
  _enable_signal(false)
{
}


void
ControlGroup::init(ControlPanel* panel, SharedPtr<PortModel> pm, bool separator)
{
	_control_panel = panel;
	_port_model = pm,
	_has_separator = separator,

	assert(_port_model);
	assert(panel);

	/*if (separator) {
	  _separator = new Gtk::VSeparator();
	  pack_start(*_separator, false, false, 4);
	  }
	  */

	pm->metadata_update_sig.connect(sigc::mem_fun(this, &ControlGroup::metadata_update));
	pm->control_change_sig.connect(sigc::mem_fun(this, &ControlGroup::set_value));
}


// ////////////////// SliderControlGroup ////////////////////// //


SliderControlGroup::SliderControlGroup(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: ControlGroup(cobject, xml),
  _enabled(true)
{
	xml->get_widget("control_strip_name_label", _name_label);
	xml->get_widget("control_strip_min_spinner", _min_spinner);
	xml->get_widget("control_strip_max_spinner", _max_spinner);
	xml->get_widget("control_strip_slider", _slider);
}


void
SliderControlGroup::init(ControlPanel* panel, SharedPtr<PortModel> pm, bool separator)
{
	ControlGroup::init(panel, pm, separator);

	assert(_name_label);
	assert(_min_spinner);
	assert(_max_spinner);
	assert(_slider);

	_slider->set_draw_value(true);

	float min = 0.0f;
	float max = 1.0f;

	const Atom& min_atom = pm->get_metadata("min");
	const Atom& max_atom = pm->get_metadata("max");
	if (min_atom.type() == Atom::FLOAT && max_atom.type() == Atom::FLOAT) {
		min = min_atom.get_float();
		max = max_atom.get_float();
	}
	
	const SharedPtr<NodeModel> parent = PtrCast<NodeModel>(pm->parent());

	if (parent && parent->plugin()->type() == PluginModel::LV2) {
		min = slv2_port_get_minimum_value(
				parent->plugin()->slv2_plugin(),
				slv2_port_by_symbol(pm->path().name().c_str()));
		max = slv2_port_get_maximum_value(
					parent->plugin()->slv2_plugin(),
					slv2_port_by_symbol(pm->path().name().c_str()));
	}

	if (max <= min)
		max = min + 1.0f;

	set_name(pm->path().name());

	_min_spinner->set_value(min);
	_min_spinner->signal_value_changed().connect(sigc::mem_fun(*this, &SliderControlGroup::min_changed));
	_max_spinner->set_value(max);
	_max_spinner->signal_value_changed().connect(sigc::mem_fun(*this, &SliderControlGroup::max_changed));

	_slider->set_value(_port_model->value());

	_slider->signal_event().connect(
			sigc::mem_fun(*this, &SliderControlGroup::slider_pressed));

	_slider->signal_value_changed().connect(
			sigc::mem_fun(*this, &SliderControlGroup::update_value_from_slider));

	_slider->set_range(min, max);

	set_value(pm->value());

	_enable_signal = true;

	show_all();
}


void
SliderControlGroup::set_value(float val)
{
	_enable_signal = false;
	//if (_enabled) {
		if (_slider->get_value() != val)
			_slider->set_value(val);
		//m_value_spinner->set_value(val);
	//}
	_enable_signal = true;
}


void
SliderControlGroup::metadata_update(const string& key, const Atom& value)
{
	_enable_signal = false;

	if ( (key == "min") && value.type() == Atom::FLOAT)
		_min_spinner->set_value(value.get_float());
	else if ( (key == "max") && value.type() == Atom::FLOAT)
		_max_spinner->set_value(value.get_float());
	
	_enable_signal = true;
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
	_min_spinner->property_sensitive() = true;
	_max_spinner->property_sensitive() = true;
	//m_value_spinner.property_sensitive() = true;
	_name_label->property_sensitive() = true;
}


void
SliderControlGroup::disable()
{
	_slider->property_sensitive() = false;
	_min_spinner->property_sensitive() = false;
	_max_spinner->property_sensitive() = false;
	//m_value_spinner.property_sensitive() = false;
	_name_label->property_sensitive() = false;
}


void
SliderControlGroup::min_changed()
{
	double       min = _min_spinner->get_value();
	const double max = _max_spinner->get_value();
	
	if (min >= max) {
		min = max - 1.0;
		_min_spinner->set_value(min);
	}

	_slider->set_range(min, max);

	if (_enable_signal) {
		char temp_buf[16];
		snprintf(temp_buf, 16, "%f", min);
		App::instance().engine()->set_metadata(_port_model->path(), "min", temp_buf);
	}
}


void
SliderControlGroup::max_changed()
{
	const double min = _min_spinner->get_value();
	double       max = _max_spinner->get_value();
	
	if (max <= min) {
		max = min + 1.0;
		_max_spinner->set_value(max);
	}

	_slider->set_range(min, max);

	if (_enable_signal) {
		char temp_buf[16];
		snprintf(temp_buf, 16, "%f", max);
		App::instance().engine()->set_metadata(_port_model->path(), "max", temp_buf);
	}
}


void
SliderControlGroup::update_value_from_slider()
{
	if (_enable_signal) {
		const float value = _slider->get_value();
		// Prevent spinner signal from doing all this over again (slow)
		_enable_signal = false;
		
		_control_panel->value_changed(_port_model, value);

		_enable_signal = true;
	}
}


/*
void
SliderControlGroup::update_value_from_spinner()
{
	if (_enable_signal) {
		_enable_signal = false;
		const float value = _value_spinner.get_value();
		
		if (value < _min_spinner->get_value()) {
			_min_spinner->set_value(value);
			_slider->set_range(_min_spinner->get_value(), _max_spinner->get_value());
		}
		if (value > _max_spinner->get_value()) {
			_max_spinner->set_value(value);
			_slider->set_range(_min_spinner->get_value(), _max_spinner->get_value());
		}

		_slider->set_value(_value_spinner.get_value());

		_control_panel->value_changed(_port_model, value);
		
		//m_port_model->value(value);
		_enable_signal = true;
	}
}
*/

/** Callback for when slider is grabbed so we can ignore set_control
 * events for this port (and avoid nasty feedback issues).
 */
bool
SliderControlGroup::slider_pressed(GdkEvent* ev)
{
	//cerr << "Pressed: " << ev->type << endl;
	if (ev->type == GDK_BUTTON_PRESS) {
		_enabled = false;
		//cerr << "SLIDER FIXME\n";
		//GtkClientInterface::instance()->set_ignore_port(_port_model->path());
	} else if (ev->type == GDK_BUTTON_RELEASE) {
		_enabled = true;
		//cerr << "SLIDER FIXME\n";
		//GtkClientInterface::instance()->clear_ignore_port();
	}
	
	return false;
}


// ///////////// IntegerControlGroup ////////////// //

#if 0
IntegerControlGroup::IntegerControlGroup(ControlPanel* panel, SharedPtr<PortModel> pm, bool separator)
: ControlGroup(panel, pm, separator),
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


ToggleControlGroup::ToggleControlGroup(ControlPanel* panel, SharedPtr<PortModel> pm, bool separator)
: ControlGroup(panel, pm, separator),
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

} // namespace Ingenuity
