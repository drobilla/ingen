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

ControlGroup::ControlGroup(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml)
: Gtk::VBox(cobject),
  m_control_panel(NULL),
  m_has_separator(false),
  m_separator(NULL),
  m_enable_signal(false)
{
}


void
ControlGroup::init(ControlPanel* panel, SharedPtr<PortModel> pm, bool separator)
{
	m_control_panel = panel;
	m_port_model = pm,
	m_has_separator = separator,

	assert(m_port_model);
	assert(panel);

	/*if (separator) {
	  m_separator = new Gtk::VSeparator();
	  pack_start(*m_separator, false, false, 4);
	  }
	  */

	pm->metadata_update_sig.connect(sigc::mem_fun(this, &ControlGroup::metadata_update));
	pm->control_change_sig.connect(sigc::mem_fun(this, &ControlGroup::set_value));
}


// ////////////////// SliderControlGroup ////////////////////// //


SliderControlGroup::SliderControlGroup(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: ControlGroup(cobject, xml),
  m_enabled(true)
{
	xml->get_widget("control_strip_name_label", m_name_label);
	xml->get_widget("control_strip_min_spinner", m_min_spinner);
	xml->get_widget("control_strip_max_spinner", m_max_spinner);
	xml->get_widget("control_strip_slider", m_slider);
}


void
SliderControlGroup::init(ControlPanel* panel, SharedPtr<PortModel> pm, bool separator)
{
	ControlGroup::init(panel, pm, separator);

	assert(m_name_label);
	assert(m_min_spinner);
	assert(m_max_spinner);
	assert(m_slider);

	m_slider->set_draw_value(true);

	float min = 0.0f;
	float max = 1.0f;

	const Atom& min_atom = pm->get_metadata("min");
	const Atom& max_atom = pm->get_metadata("max");
	if (min_atom.type() == Atom::FLOAT && max_atom.type() == Atom::FLOAT) {
		min = min_atom.get_float();
		max = max_atom.get_float();
	}

	if (max <= min)
		max = min + 1.0f;

	set_name(pm->path().name());
	
	m_min_spinner->set_value(min);
	m_min_spinner->signal_value_changed().connect(sigc::mem_fun(*this, &SliderControlGroup::min_changed));
	m_max_spinner->set_value(max);
	m_max_spinner->signal_value_changed().connect(sigc::mem_fun(*this, &SliderControlGroup::max_changed));

	m_slider->set_value(m_port_model->value());

	m_slider->signal_event().connect(
		sigc::mem_fun(*this, &SliderControlGroup::slider_pressed));

	m_slider->signal_value_changed().connect(
		sigc::mem_fun(*this, &SliderControlGroup::update_value_from_slider));
	
	m_slider->set_range(min, max);

	set_value(pm->value());

	m_enable_signal = true;

	show_all();
}


void
SliderControlGroup::metadata_update(const string& key, const Atom& value)
{
	m_enable_signal = false;

	if ( (key == "min") && value.type() == Atom::FLOAT)
		m_min_spinner->set_value(value.get_float());
	else if ( (key == "max") && value.type() == Atom::FLOAT)
		m_max_spinner->set_value(value.get_float());
	
	m_enable_signal = true;
}


void
SliderControlGroup::set_name(const string& name)
{
	string name_label = "<span weight=\"bold\">";
	name_label += name + "</span>";
	m_name_label->set_markup(name_label);
}


void
SliderControlGroup::enable()
{
	m_slider->property_sensitive() = true;
	m_min_spinner->property_sensitive() = true;
	m_max_spinner->property_sensitive() = true;
	//m_value_spinner.property_sensitive() = true;
	m_name_label->property_sensitive() = true;
}


void
SliderControlGroup::disable()
{
	m_slider->property_sensitive() = false;
	m_min_spinner->property_sensitive() = false;
	m_max_spinner->property_sensitive() = false;
	//m_value_spinner.property_sensitive() = false;
	m_name_label->property_sensitive() = false;
}


void
SliderControlGroup::min_changed()
{
	double       min = m_min_spinner->get_value();
	const double max = m_max_spinner->get_value();
	
	if (min >= max) {
		min = max - 1.0;
		m_min_spinner->set_value(min);
	}

	m_slider->set_range(min, max);

	if (m_enable_signal) {
		char temp_buf[16];
		snprintf(temp_buf, 16, "%f", min);
		App::instance().engine()->set_metadata(m_port_model->path(), "min", temp_buf);
	}
}


void
SliderControlGroup::max_changed()
{
	const double min = m_min_spinner->get_value();
	double       max = m_max_spinner->get_value();
	
	if (max <= min) {
		max = min + 1.0;
		m_max_spinner->set_value(max);
	}

	m_slider->set_range(min, max);

	if (m_enable_signal) {
		char temp_buf[16];
		snprintf(temp_buf, 16, "%f", max);
		App::instance().engine()->set_metadata(m_port_model->path(), "max", temp_buf);
	}
}


void
SliderControlGroup::update_value_from_slider()
{
	if (m_enable_signal) {
		const float value = m_slider->get_value();
		// Prevent spinner signal from doing all this over again (slow)
		m_enable_signal = false;
		
		m_control_panel->value_changed(m_port_model, value);

		m_enable_signal = true;
	}
}


/*
void
SliderControlGroup::update_value_from_spinner()
{
	if (m_enable_signal) {
		m_enable_signal = false;
		const float value = m_value_spinner.get_value();
		
		if (value < m_min_spinner->get_value()) {
			m_min_spinner->set_value(value);
			m_slider->set_range(m_min_spinner->get_value(), m_max_spinner->get_value());
		}
		if (value > m_max_spinner->get_value()) {
			m_max_spinner->set_value(value);
			m_slider->set_range(m_min_spinner->get_value(), m_max_spinner->get_value());
		}

		m_slider->set_value(m_value_spinner.get_value());

		m_control_panel->value_changed(m_port_model, value);
		
		//m_port_model->value(value);
		m_enable_signal = true;
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

#if 0
IntegerControlGroup::IntegerControlGroup(ControlPanel* panel, SharedPtr<PortModel> pm, bool separator)
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
	m_name_label->set_markup(name_label);
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
	m_name_label->property_sensitive() = true;
}


void
IntegerControlGroup::disable()
{
	m_spinner.property_sensitive() = false;
	m_name_label->property_sensitive() = false;
}


void
IntegerControlGroup::update_value()
{
	if (m_enable_signal) {
		float value = m_spinner.get_value();
		m_control_panel->value_changed(m_port_model, value);
		//m_port_model->value(value);
	}
}


// ///////////// ToggleControlGroup ////////////// //


ToggleControlGroup::ToggleControlGroup(ControlPanel* panel, SharedPtr<PortModel> pm, bool separator)
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
	m_name_label->set_markup(name_label);
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
	m_name_label->property_sensitive() = true;
}


void
ToggleControlGroup::disable()
{
	m_checkbutton.property_sensitive() = false;
	m_name_label->property_sensitive() = false;
}


void
ToggleControlGroup::update_value()
{
	if (m_enable_signal) {
		float value = m_checkbutton.get_active() ? 1.0f : 0.0f;
		m_control_panel->value_changed(m_port_model, value);
		//m_port_model->value(value);
	}
}
#endif

} // namespace Ingenuity
