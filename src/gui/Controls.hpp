/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_GUI_CONTROLS_HPP
#define INGEN_GUI_CONTROLS_HPP

#include <cassert>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include <libglademm.h>
#include "client/PortModel.hpp"
#include "raul/SharedPtr.hpp"

namespace Ingen { namespace Client { class PortModel; } }

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

	virtual void init(ControlPanel* panel, SharedPtr<Client::PortModel> pm);

	void enable();
	void disable();

	inline const SharedPtr<Client::PortModel> port_model() const { return _port_model; }

protected:
	virtual void set_value(const Raul::Atom& value) = 0;
	virtual void set_range(float min, float max) {}

	void set_name(const std::string& name);
	void menu_properties();

	ControlPanel*                _control_panel;
	SharedPtr<Client::PortModel> _port_model;
	sigc::connection             _control_connection;
	bool                         _enable_signal;

	Gtk::Menu*     _menu;
	Gtk::MenuItem* _menu_properties;
	Gtk::Label*    _name_label;
};


/** A slider for a continuous control.
 *
 * \ingroup GUI
 */
class SliderControl : public Control
{
public:
	SliderControl(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml);
	void init(ControlPanel* panel, SharedPtr<Client::PortModel> pm);

	void set_min(float val);
	void set_max(float val);

private:
	void set_value(const Raul::Atom& value);
	void set_range(float min, float max);

	void port_property_changed(const Raul::URI& key, const Raul::Atom& value);

	void update_range();
	void update_value_from_slider();
	void update_value_from_spinner();

	bool slider_pressed(GdkEvent* ev);
	bool clicked(GdkEventButton* ev);

	bool _enabled;

	Gtk::SpinButton* _value_spinner;
	Gtk::HScale*     _slider;
};


/** A radio button for toggle controls.
 *
 * \ingroup GUI
 */
class ToggleControl : public Control
{
public:
	ToggleControl(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);

	void init(ControlPanel* panel, SharedPtr<Client::PortModel> pm);

private:
	void set_value(const Raul::Atom& value);
	void toggled();

	Gtk::CheckButton* _checkbutton;
};


/** A text entry for string controls.
 *
 * \ingroup GUI
 */
class StringControl : public Control
{
public:
	StringControl(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);

	void init(ControlPanel* panel, SharedPtr<Client::PortModel> pm);

private:
	void set_value(const Raul::Atom& value);
	void activated();

	Gtk::Entry* _entry;
};


} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_CONTROLS_HPP
