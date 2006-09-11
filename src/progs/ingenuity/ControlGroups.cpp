/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
#include "PortModel.h"
#include "App.h"

using std::cerr; using std::cout; using std::endl;

using namespace Ingen::Client;

namespace Ingenuity {


// ////////////////////// ControlGroup ///////////////////////////////// //

ControlGroup::ControlGroup(ControlPanel* panel, CountedPtr<PortModel> pm, bool separator)
: Gtk::VBox(false, 0),
  m_control_panel(panel),
  m_port_model(pm),
  m_has_separator(separator)
{
	assert(m_port_model);
	assert(panel);

	if (separator) {
		m_separator = new Gtk::HSeparator();
		pack_start(*m_separator, false, false, 4);
	} else {
		m_separator = NULL;
	}
	
	pm->metadata_update_sig.connect(sigc::mem_fun(this, &ControlGroup::metadata_update));
	pm->control_change_sig.connect(sigc::mem_fun(this, &ControlGroup::set_value));
}


void
ControlGroup::metadata_update(const string& key, const string& value)
{
	// FIXME: this isn't right
	if (key == "user-min" || key == "min")
		set_min(atof(value.c_str()));
	else if (key == "user-max" || key == "max")
		set_max(atof(value.c_str()));
}


// ////////////////// SliderControlGroup ////////////////////// //

SliderControlGroup::SliderControlGroup(ControlPanel* panel, CountedPtr<PortModel> pm, bool separator)
: ControlGroup(panel, pm, separator),
  m_enabled(true),
  m_enable_signal(false),
  m_name_label(pm->path().name(), 0.0, 0.0),
  m_range_box(false, 0),
  m_range_label("<small>Range: </small>"),
  m_min_spinner(1.0, (pm->is_integer() ? 0 : 4)), // climb rate, digits
  m_hyphen_label(" - "),
  m_max_spinner(1.0, (pm->is_integer() ? 0 : 4)),
  m_slider_box(false, 0),
  m_value_spinner(1.0, (pm->is_integer() ? 0 : 4)),
  m_slider(pm->user_min(), pm->user_max(), (pm->is_integer() ? 1.0 : 0.0001))
{
	m_slider.set_increments(1.0, 10.0);

	// Compensate for crazy plugins
	// FIXME
	/*
	if (m_port_model->user_max() <= m_port_model->user_min()) {
		m_port_model->user_max(m_port_model->user_min() + 1.0);
		m_slider.set_range(m_port_model->user_min(), m_port_model->user_max());
	}*/
	m_slider.property_draw_value() = false;
	
	set_name(pm->path().name());
	
	m_name_label.property_use_markup() = true;
	m_range_label.property_use_markup() = true;

	m_value_spinner.set_range(-99999, 99999);
	m_value_spinner.set_update_policy(Gtk::UPDATE_ALWAYS); // allow entered values outside range
	m_value_spinner.set_value(m_port_model->value());
	m_value_spinner.set_increments(1.0, 10.0);
	m_value_spinner.set_digits(5);
	m_value_spinner.signal_value_changed().connect(
		sigc::mem_fun(*this, &SliderControlGroup::update_value_from_spinner));
	m_min_spinner.set_range(-99999, 99999);
	m_min_spinner.set_value(m_port_model->user_min());
	m_min_spinner.set_increments(1.0, 10.0);
	m_min_spinner.set_digits(5);
	m_min_spinner.signal_value_changed().connect(sigc::mem_fun(*this, &SliderControlGroup::min_changed));
	m_max_spinner.set_range(-99999, 99999);
	m_max_spinner.set_value(m_port_model->user_max());
	m_max_spinner.set_increments(1.0, 10.0);
	m_max_spinner.set_digits(5);
	m_max_spinner.signal_value_changed().connect(sigc::mem_fun(*this, &SliderControlGroup::max_changed));

	m_slider.set_value(m_port_model->value());

	m_slider.signal_event().connect(
		sigc::mem_fun(*this, &SliderControlGroup::slider_pressed));

	m_slider.signal_value_changed().connect(
		sigc::mem_fun(*this, &SliderControlGroup::update_value_from_slider));

	m_range_box.pack_start(m_range_label, false, false, 2);
	m_range_box.pack_start(m_min_spinner, false, false, 1);
	m_range_box.pack_start(m_hyphen_label, false, false, 1);
	m_range_box.pack_start(m_max_spinner, false, false, 1);
	
	m_header_box.pack_start(m_name_label, true, true, 0);
	m_header_box.pack_start(m_range_box, true, true, 2);
	
	m_slider_box.pack_start(m_value_spinner, false, false, 1);
	m_slider_box.pack_start(m_slider, true, true, 5);
	
	pack_start(m_header_box, false, false, 0);
	pack_start(m_slider_box, false, false, 0);
	
	m_slider.set_range(m_port_model->user_min(), m_port_model->user_max());

	m_enable_signal = true;

	show_all();
}


void
SliderControlGroup::set_name(const string& name)
{
	string name_label = "<small><span weight=\"bold\">";
	name_label += name + "</span></small>";
	m_name_label.set_markup(name_label);
}


void
SliderControlGroup::set_min(float val)
{
	m_enable_signal = false;
	m_min_spinner.set_value(val);
	m_enable_signal = true;
}


void
SliderControlGroup::set_max(float val)
{
	m_enable_signal = false;
	m_max_spinner.set_value(val);
	m_enable_signal = true;
}


void
SliderControlGroup::enable()
{
	m_slider.property_sensitive() = true;
	m_min_spinner.property_sensitive() = true;
	m_max_spinner.property_sensitive() = true;
	m_value_spinner.property_sensitive() = true;
	m_name_label.property_sensitive() = true;
	m_range_label.property_sensitive() = true;
}


void
SliderControlGroup::disable()
{
	m_slider.property_sensitive() = false;
	m_min_spinner.property_sensitive() = false;
	m_max_spinner.property_sensitive() = false;
	m_value_spinner.property_sensitive() = false;
	m_name_label.property_sensitive() = false;
	m_range_label.property_sensitive() = false;
}


void
SliderControlGroup::min_changed()
{
	double       min = m_min_spinner.get_value();
	const double max = m_max_spinner.get_value();
	
	if (min >= max) {
		min = max - 1.0;
		m_min_spinner.set_value(min);
	}

	m_slider.set_range(min, max);

	if (m_enable_signal) {
		char temp_buf[16];
		snprintf(temp_buf, 16, "%f", min);
		App::instance().engine()->set_metadata(m_port_model->path(), "user-min", temp_buf);
	}
}


void
SliderControlGroup::max_changed()
{
	const double min = m_min_spinner.get_value();
	double       max = m_max_spinner.get_value();
	
	if (max <= min) {
		max = min + 1.0;
		m_max_spinner.set_value(max);
	}

	m_slider.set_range(min, max);

	if (m_enable_signal) {
		char temp_buf[16];
		snprintf(temp_buf, 16, "%f", max);
		App::instance().engine()->set_metadata(m_port_model->path(), "user-max", temp_buf);
	}
}


void
SliderControlGroup::update_value_from_slider()
{
	if (m_enable_signal) {
		const float value = m_slider.get_value();
		// Prevent spinner signal from doing all this over again (slow)
		m_enable_signal = false;
		m_value_spinner.set_value(value);
		m_control_panel->value_changed(m_port_model->path(), value);
		m_port_model->value(value);
		m_enable_signal = true;
	}
}


void
SliderControlGroup::update_value_from_spinner()
{
	if (m_enable_signal) {
		m_enable_signal = false;
		const float value = m_value_spinner.get_value();
		
		if (value < m_min_spinner.get_value()) {
			m_min_spinner.set_value(value);
			m_slider.set_range(m_min_spinner.get_value(), m_max_spinner.get_value());
		}
		if (value > m_max_spinner.get_value()) {
			m_max_spinner.set_value(value);
			m_slider.set_range(m_min_spinner.get_value(), m_max_spinner.get_value());
		}

		m_slider.set_value(m_value_spinner.get_value());

		m_control_panel->value_changed(m_port_model->path(), value);
		
		m_port_model->value(value);
		m_enable_signal = true;
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
		m_enabled = false;
		//cerr << "SLIDER FIXME\n";
		//GtkClientInterface::instance()->set_ignore_port(m_port_model->path());
	} else if (ev->type == GDK_BUTTON_RELEASE) {
		m_enabled = true;
		//cerr << "SLIDER FIXME\n";
		//GtkClientInterface::instance()->clear_ignore_port();
	}
	
	return false;
}


// ///////////// IntegerControlGroup ////////////// //


IntegerControlGroup::IntegerControlGroup(ControlPanel* panel, CountedPtr<PortModel> pm, bool separator)
: ControlGroup(panel, pm, separator),
  m_enable_signal(false),
  m_alignment(0.5, 0.5, 0.0, 0.0),
  m_name_label(pm->path().name()),
  m_spinner(1.0, 0)
{
	set_name(pm->path().name());

	m_spinner.set_range(-99999, 99999);
	m_spinner.set_value(m_port_model->value());
	m_spinner.signal_value_changed().connect(
		sigc::mem_fun(*this, &IntegerControlGroup::update_value));
	m_spinner.set_increments(1, 10);
	
	m_alignment.add(m_spinner);
	pack_start(m_name_label);
	pack_start(m_alignment);

	m_enable_signal = true;

	show_all();
}


void
IntegerControlGroup::set_name(const string& name)
{
	string name_label = "<span weight=\"bold\">";
	name_label += name + "</span>";
	m_name_label.set_markup(name_label);
}


void
IntegerControlGroup::set_value(float val)
{
	//cerr << "[IntegerControlGroup] Setting value to " << val << endl;
	m_enable_signal = false;
	m_spinner.set_value(val);
	m_enable_signal = true;
}


void
IntegerControlGroup::enable()
{
	m_spinner.property_sensitive() = true;
	m_name_label.property_sensitive() = true;
}


void
IntegerControlGroup::disable()
{
	m_spinner.property_sensitive() = false;
	m_name_label.property_sensitive() = false;
}


void
IntegerControlGroup::update_value()
{
	if (m_enable_signal) {
		float value = m_spinner.get_value();
		m_control_panel->value_changed(m_port_model->path(), value);
		m_port_model->value(value);
	}
}


// ///////////// ToggleControlGroup ////////////// //


ToggleControlGroup::ToggleControlGroup(ControlPanel* panel, CountedPtr<PortModel> pm, bool separator)
: ControlGroup(panel, pm, separator),
  m_enable_signal(false),
  m_alignment(0.5, 0.5, 0.0, 0.0),
  m_name_label(pm->path().name())
{
	set_name(pm->path().name());

	set_value(m_port_model->value());
	m_checkbutton.signal_toggled().connect(
		sigc::mem_fun(*this, &ToggleControlGroup::update_value));
	
	m_alignment.add(m_checkbutton);
	pack_start(m_name_label);
	pack_start(m_alignment);

	m_enable_signal = true;

	show_all();
}


void
ToggleControlGroup::set_name(const string& name)
{
	string name_label = "<span weight=\"bold\">";
	name_label += name + "</span>";
	m_name_label.set_markup(name_label);
}


void
ToggleControlGroup::set_value(float val)
{
	//cerr << "[ToggleControlGroup] Setting value to " << val << endl;
	m_enable_signal = false;
	m_checkbutton.set_active( (val > 0.0f) );
	m_enable_signal = true;
}


void
ToggleControlGroup::enable()
{
	m_checkbutton.property_sensitive() = true;
	m_name_label.property_sensitive() = true;
}


void
ToggleControlGroup::disable()
{
	m_checkbutton.property_sensitive() = false;
	m_name_label.property_sensitive() = false;
}


void
ToggleControlGroup::update_value()
{
	if (m_enable_signal) {
		float value = m_checkbutton.get_active() ? 1.0f : 0.0f;
		m_control_panel->value_changed(m_port_model->path(), value);
		m_port_model->value(value);
	}
}


} // namespace Ingenuity
