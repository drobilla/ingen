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

#ifndef CONTROLGROUPS_H
#define CONTROLGROUPS_H

#include <cassert>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include <libglademm.h>
#include "PortModel.h"
#include "raul/SharedPtr.h"

namespace Ingen { namespace Client { class PortModel; } }
using namespace Ingen::Client;

namespace Ingenuity {

class ControlPanel;


/** A group of controls (for a single Port) in a NodeControlWindow.
 *
 * \ingroup Ingenuity
 */
class ControlGroup : public Gtk::VBox
{
public:
	ControlGroup(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml);
	
	void init(ControlPanel* panel, SharedPtr<PortModel> pm, bool separator);

	~ControlGroup() { delete m_separator; }
	
	inline const SharedPtr<PortModel> port_model() const { return m_port_model; }

	void remove_separator() {
		assert(m_has_separator); remove(*m_separator); delete m_separator;
	}

protected:

	virtual void set_value(float value) = 0;
	virtual void set_min(float min) = 0;
	virtual void set_max(float max) = 0;
	virtual void metadata_update(const string& key, const Atom& value);

	ControlPanel*        m_control_panel;
	SharedPtr<PortModel> m_port_model;
	bool                 m_has_separator;
	Gtk::VSeparator*     m_separator;
};


/** A slider for a continuous control.
 *
 * \ingroup Ingenuity
 */
class SliderControlGroup : public ControlGroup
{
public:
	SliderControlGroup(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml);
	void init(ControlPanel* panel, SharedPtr<PortModel> pm, bool separator);

	void enable();
	void disable();

private:
	void set_name(const string& name);

	inline void set_value(const float val);
	void set_min(float val);
	void set_max(float val);

	void min_changed();
	void max_changed();
	void update_range();
	void update_value_from_slider();
	void update_value_from_spinner();
	
	//void slider_grabbed(bool b);

	bool slider_pressed(GdkEvent* ev);

	bool m_enabled;
	bool m_enable_signal;
	
	Gtk::Label*      m_name_label;
	Gtk::SpinButton* m_min_spinner;
	Gtk::SpinButton* m_max_spinner;
	//Gtk::SpinButton* m_value_spinner;
	Gtk::VScale*     m_slider;
};


inline void
SliderControlGroup::set_value(const float val)
{
	m_enable_signal = false;
	if (m_enabled) {
		m_slider->set_value(val);
		//m_value_spinner->set_value(val);
	}
	m_enable_signal = true;
}


#if 0

/** A spinbutton for integer controls.
 * 
 * \ingroup Ingenuity
 */
class IntegerControlGroup : public ControlGroup
{
public:
	IntegerControlGroup(ControlPanel* panel, SharedPtr<PortModel> pm, bool separator);
	
	void enable();
	void disable();
	
private:
	void set_name(const string& name);
	void set_value(float val);

	void update_value();
	
	bool            m_enable_signal;
	Gtk::Alignment  m_alignment;
	Gtk::Label      m_name_label;
	Gtk::SpinButton m_spinner;
};


/** A radio button for toggle controls.
 * 
 * \ingroup Ingenuity
 */
class ToggleControlGroup : public ControlGroup
{
public:
	ToggleControlGroup(ControlPanel* panel, SharedPtr<PortModel> pm, bool separator);
	
	void enable();
	void disable();
	
private:
	void set_name(const string& name);
	void set_value(float val);

	void update_value();
	
	bool             m_enable_signal;
	Gtk::Alignment   m_alignment;
	Gtk::Label       m_name_label;
	Gtk::CheckButton m_checkbutton;
};
#endif

} // namespace Ingenuity

#endif // CONTROLGROUPS_H
