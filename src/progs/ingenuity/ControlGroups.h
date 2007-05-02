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

#ifndef CONTROLGROUPS_H
#define CONTROLGROUPS_H

#include <cassert>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include <libglademm.h>
#include "client/PortModel.h"
#include <raul/SharedPtr.h>

namespace Ingen { namespace Client { class PortModel; } }
using namespace Ingen::Client;

namespace Ingenuity {

class ControlPanel;
class PortPropertiesWindow;


/** A group of controls (for a single Port) in a NodeControlWindow.
 *
 * \ingroup Ingenuity
 */
class ControlGroup : public Gtk::VBox
{
public:
	ControlGroup(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml);
	virtual ~ControlGroup() { }
	
	void init(ControlPanel* panel, SharedPtr<PortModel> pm);
	
	inline const SharedPtr<PortModel> port_model() const { return _port_model; }

protected:
	friend class PortPropertiesWindow;

	virtual void set_value(float value) = 0;
	virtual void set_range(float min, float max) {}

	ControlPanel*        _control_panel;
	SharedPtr<PortModel> _port_model;
	bool                 _enable_signal;
};


/** A slider for a continuous control.
 *
 * \ingroup Ingenuity
 */
class SliderControlGroup : public ControlGroup
{
public:
	SliderControlGroup(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml);
	void init(ControlPanel* panel, SharedPtr<PortModel> pm);

	void enable();
	void disable();
	
	void set_min(float val);
	void set_max(float val);

private:
	void set_name(const string& name);

	bool clicked(GdkEventButton* ev);

	void set_value(float value);
	void set_range(float min, float max);

	void update_range();
	void update_value_from_slider();
	void update_value_from_spinner();

	void menu_properties();
	
	bool slider_pressed(GdkEvent* ev);

	bool _enabled;
	
	Gtk::Label*      _name_label;
	Gtk::SpinButton* _value_spinner;
	Gtk::HScale*     _slider;
	
	Gtk::Menu*      _menu;
	Gtk::MenuItem*  _menu_properties;
};


#if 0

/** A spinbutton for integer controls.
 * 
 * \ingroup Ingenuity
 */
class IntegerControlGroup : public ControlGroup
{
public:
	IntegerControlGroup(ControlPanel* panel, SharedPtr<PortModel> pm);
	
	void enable();
	void disable();
	
private:
	void set_name(const string& name);
	void set_value(float val);

	void update_value();
	
	bool            _enable_signal;
	Gtk::Alignment  _alignment;
	Gtk::Label      _name_label;
	Gtk::SpinButton _spinner;
};


/** A radio button for toggle controls.
 * 
 * \ingroup Ingenuity
 */
class ToggleControlGroup : public ControlGroup
{
public:
	ToggleControlGroup(ControlPanel* panel, SharedPtr<PortModel> pm);
	
	void enable();
	void disable();
	
private:
	void set_name(const string& name);
	void set_value(float val);

	void update_value();
	
	bool             _enable_signal;
	Gtk::Alignment   _alignment;
	Gtk::Label       _name_label;
	Gtk::CheckButton _checkbutton;
};
#endif

} // namespace Ingenuity

#endif // CONTROLGROUPS_H
