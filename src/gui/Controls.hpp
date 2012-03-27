/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_CONTROLS_HPP
#define INGEN_GUI_CONTROLS_HPP

#include <cassert>

#include <gtkmm.h>

#include "raul/SharedPtr.hpp"

#include "ingen/client/PortModel.hpp"

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
	Control(BaseObjectType*                   cobject,
	        const Glib::RefPtr<Gtk::Builder>& xml);
	virtual ~Control();

	virtual void init(App&                               app,
	                  ControlPanel*                      panel,
	                  SharedPtr<const Client::PortModel> pm);

	void enable();
	void disable();

	inline const SharedPtr<const Client::PortModel> port_model() const { return _port_model; }

protected:
	virtual void set_value(const Raul::Atom& value) = 0;
	virtual void set_range(float min, float max) {}

	void set_label(const std::string& name);
	void menu_properties();

	App*                               _app;
	ControlPanel*                      _control_panel;
	SharedPtr<const Client::PortModel> _port_model;
	sigc::connection                   _control_connection;
	Gtk::Menu*                         _menu;
	Gtk::MenuItem*                     _menu_properties;
	Gtk::Label*                        _name_label;
	bool                               _enable_signal;
};

/** A slider for a continuous control.
 *
 * \ingroup GUI
 */
class SliderControl : public Control
{
public:
	SliderControl(BaseObjectType*                   cobject,
	              const Glib::RefPtr<Gtk::Builder>& xml);

	void init(App&                               app,
	          ControlPanel*                      panel,
	          SharedPtr<const Client::PortModel> pm);

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
	ToggleControl(BaseObjectType*                   cobject,
	              const Glib::RefPtr<Gtk::Builder>& xml);

	void init(App&                               app,
	          ControlPanel*                      panel,
	          SharedPtr<const Client::PortModel> pm);

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
	StringControl(BaseObjectType*                   cobject,
	              const Glib::RefPtr<Gtk::Builder>& xml);

	void init(App&                               app,
	          ControlPanel*                      panel,
	          SharedPtr<const Client::PortModel> pm);

private:
	void set_value(const Raul::Atom& value);
	void activated();

	Gtk::Entry* _entry;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_CONTROLS_HPP
