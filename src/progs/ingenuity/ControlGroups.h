/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef CONTROLGROUPS_H
#define CONTROLGROUPS_H

#include <cassert>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include <libglademm.h>
#include "PortModel.h"
#include "util/CountedPtr.h"


namespace LibOmClient { class PortModel; }
using namespace LibOmClient;

namespace OmGtk {

class ControlPanel;


/** A group of controls in a NodeControlWindow.
 *
 * \ingroup OmGtk
 */
class ControlGroup : public Gtk::VBox
{
public:
	ControlGroup(ControlPanel* panel, CountedPtr<PortModel> pm, bool separator)
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
	}

	~ControlGroup() { delete m_separator; }
	
	virtual void set_name(const string& name) {}
	
	virtual void set_value(float val) = 0;
	
	inline const CountedPtr<PortModel> port_model() const { return m_port_model; }

	virtual void set_min(float val) {}
	virtual void set_max(float val) {}

	void remove_separator() { assert(m_has_separator);
		remove(*m_separator); delete m_separator; }

	virtual void enable() {}
	virtual void disable() {}
	
protected:
	ControlPanel*             m_control_panel;
	CountedPtr<PortModel>                m_port_model;
	bool                      m_has_separator;
	Gtk::HSeparator*          m_separator;
};


/** A slider for a continuous control.
 *
 * \ingroup OmGtk
 */
class SliderControlGroup : public ControlGroup
{
public:
	SliderControlGroup(ControlPanel* panel, CountedPtr<PortModel> pm, bool separator);
	
	void set_name(const string& name);

	inline void set_value(const float val);
	void set_min(float val);
	void set_max(float val);

	void enable();
	void disable();

private:
	void min_changed();
	void max_changed();
	void update_range();
	void update_value_from_slider();
	void update_value_from_spinner();
	
	//void slider_grabbed(bool b);

	bool slider_pressed(GdkEvent* ev);

	bool m_enabled;
	bool m_enable_signal;
	
	Gtk::HBox       m_header_box;
	Gtk::Label      m_name_label;
	Gtk::HBox       m_range_box;
	Gtk::Label      m_range_label;
	Gtk::SpinButton m_min_spinner;
	Gtk::Label      m_hyphen_label;
	Gtk::SpinButton m_max_spinner;
	Gtk::HBox       m_slider_box;
	Gtk::SpinButton m_value_spinner;
	Gtk::HScale     m_slider;
};


inline void
SliderControlGroup::set_value(const float val)
{
	m_enable_signal = false;
	if (m_enabled) {
		m_slider.set_value(val);
		m_value_spinner.set_value(val);
	}
	m_port_model->value(val);
	m_enable_signal = true;
}




/** A spinbutton for integer controls.
 * 
 * \ingroup OmGtk
 */
class IntegerControlGroup : public ControlGroup
{
public:
	IntegerControlGroup(ControlPanel* panel, CountedPtr<PortModel> pm, bool separator);
	
	void set_name(const string& name);
	void set_value(float val);

	void enable();
	void disable();
	
private:
	void update_value();
	
	bool            m_enable_signal;
	Gtk::Alignment  m_alignment;
	Gtk::Label      m_name_label;
	Gtk::SpinButton m_spinner;
};


/** A radio button for toggle controls.
 * 
 * \ingroup OmGtk
 */
class ToggleControlGroup : public ControlGroup
{
public:
	ToggleControlGroup(ControlPanel* panel, CountedPtr<PortModel> pm, bool separator);
	
	void set_name(const string& name);
	void set_value(float val);

	void enable();
	void disable();
	
private:
	void update_value();
	
	bool             m_enable_signal;
	Gtk::Alignment   m_alignment;
	Gtk::Label       m_name_label;
	Gtk::CheckButton m_checkbutton;
};


} // namespace OmGtk

#endif // CONTROLGROUPS_H
