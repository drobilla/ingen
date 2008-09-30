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
#include "client/PortModel.hpp"
#include <raul/SharedPtr.hpp>

namespace Ingen { namespace Client { class PortModel; } }
using namespace Ingen::Client;

namespace Ingen {
namespace GUI {

class ControlPanel;


/** A group of controls (for a single Port) in a ControlPanel.
 *
 * \ingroup GUI
 */
class Control : public Gtk::VBox
{
public:
	Control(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml);
	virtual ~Control();
	
	virtual void init(ControlPanel* panel, SharedPtr<PortModel> pm);
	
	virtual void enable()  = 0;
	virtual void disable() = 0;
	
	inline const SharedPtr<PortModel> port_model() const { return _port_model; }

protected:
	virtual void set_value(const Atom& value) = 0;
	virtual void set_range(float min, float max) {}
	
	void menu_properties();

	ControlPanel*        _control_panel;
	SharedPtr<PortModel> _port_model;
	sigc::connection     _control_connection;
	bool                 _enable_signal;
	
	Gtk::Menu*     _menu;
	Gtk::MenuItem* _menu_properties;
};


/** A slider for a continuous control.
 *
 * \ingroup GUI
 */
class SliderControl : public Control
{
public:
	SliderControl(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml);
	void init(ControlPanel* panel, SharedPtr<PortModel> pm);

	void enable();
	void disable();
	
	void set_min(float val);
	void set_max(float val);

private:
	void set_name(const string& name);
	void set_value(const Atom& value);
	void set_range(float min, float max);
	
	void port_variable_change(const string& key, const Raul::Atom& value);

	void update_range();
	void update_value_from_slider();
	void update_value_from_spinner();
	
	bool slider_pressed(GdkEvent* ev);
	bool clicked(GdkEventButton* ev);

	bool _enabled;
	
	Gtk::Label*      _name_label;
	Gtk::SpinButton* _value_spinner;
	Gtk::HScale*     _slider;
};


#if 0

/** A spinbutton for integer controls.
 * 
 * \ingroup GUI
 */
class IntegerControl : public Control
{
public:
	IntegerControl(ControlPanel* panel, SharedPtr<PortModel> pm);
	
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
#endif


/** A radio button for toggle controls.
 * 
 * \ingroup GUI
 */
class ToggleControl : public Control
{
public:
	ToggleControl(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);
	
	void init(ControlPanel* panel, SharedPtr<PortModel> pm);
	
	void enable();
	void disable();
	
private:
	void set_name(const string& name);
	void set_value(const Atom& value);

	void toggled();
	
	Gtk::Label*       _name_label;
	Gtk::CheckButton* _checkbutton;
};


} // namespace GUI
} // namespace Ingen

#endif // CONTROLGROUPS_H
