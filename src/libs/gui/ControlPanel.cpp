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
 * You should have received a copy of the GNU General Public License alongCont
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "interface/EngineInterface.hpp"
#include "client/PatchModel.hpp"
#include "client/NodeModel.hpp"
#include "client/PortModel.hpp"
#include "client/PluginModel.hpp"
#include "App.hpp"
#include "ControlPanel.hpp"
#include "Controls.hpp"
#include "GladeFactory.hpp"

namespace Ingen {
namespace GUI {

	
ControlPanel::ControlPanel(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Gtk::HBox(cobject)
	, _callback_enabled(true)
{
	xml->get_widget("control_panel_controls_box", _control_box);
	xml->get_widget("control_panel_voice_controls_box", _voice_control_box);
	xml->get_widget("control_panel_all_voices_radio", _all_voices_radio);
	xml->get_widget("control_panel_specific_voice_radio", _specific_voice_radio);
	xml->get_widget("control_panel_voice_spinbutton", _voice_spinbutton);
	
	_all_voices_radio->signal_toggled().connect(
			sigc::mem_fun(this, &ControlPanel::all_voices_selected));

	_specific_voice_radio->signal_toggled().connect(
			sigc::mem_fun(this, &ControlPanel::specific_voice_selected));

	show_all();
}


ControlPanel::~ControlPanel()
{
	for (vector<Control*>::iterator i = _controls.begin(); i != _controls.end(); ++i)
		delete (*i);
}


void
ControlPanel::init(SharedPtr<NodeModel> node, uint32_t poly)
{
	assert(node != NULL);
	assert(poly > 0);
	
	if (node->polyphonic()) {
		_voice_spinbutton->set_range(0, poly - 1);
		_voice_control_box->show();
	} else {
		_voice_control_box->hide();
	}

	for (PortModelList::const_iterator i = node->ports().begin(); i != node->ports().end(); ++i) {
		add_port(*i);
	}
		
	node->signal_polyphonic.connect(
		sigc::mem_fun(this, &ControlPanel::polyphonic_changed));

	if (node->parent()) {
		((PatchModel*)node->parent().get())->signal_polyphony.connect(
				sigc::mem_fun(this, &ControlPanel::polyphony_changed));
	} else {
		cerr << "[ControlPanel] No parent, polyphonic controls disabled" << endl;
	}
	
	_callback_enabled = true;
}


Control*
ControlPanel::find_port(const Path& path) const
{
	for (vector<Control*>::const_iterator cg = _controls.begin(); cg != _controls.end(); ++cg)
		if ((*cg)->port_model()->path() == path)
			return (*cg);

	return NULL;
}


/** Add a control to the panel for the given port.
 */
void
ControlPanel::add_port(SharedPtr<PortModel> pm)
{
	assert(pm);
	
	// Already have port, don't add another
	if (find_port(pm->path()) != NULL)
		return;
	
	// Add port
	if (pm->type().is_control() && pm->is_input()) {
		Control* control = NULL;
			
		if (pm->is_toggle()) {
			ToggleControl* tc;
			Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference("toggle_control");
			xml->get_widget_derived("toggle_control", tc);
			control = tc;
		} else {
			SliderControl* sc;
			Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference("control_strip");
			xml->get_widget_derived("control_strip", sc);
			control = sc;
		}
			
		control->init(this, pm);
	
		if (_controls.size() > 0)
			_control_box->pack_start(*Gtk::manage(new Gtk::HSeparator()), false, false, 4);
		
		_controls.push_back(control);
		_control_box->pack_start(*control, false, false, 0);

		control->enable();
		control->show();
	}

	Gtk::Requisition controls_size;
	_control_box->size_request(controls_size);
	_ideal_size.first = controls_size.width;
	_ideal_size.second = controls_size.height;
	
	Gtk::Requisition voice_size;
	_voice_control_box->size_request(voice_size);
	_ideal_size.first += voice_size.width;
	_ideal_size.second += voice_size.height;

	//cerr << "Setting ideal size to " << _ideal_size.first
	//	<< " x " << _ideal_size.second << endl;
}


/** Remove the control for the given port.
 */
void
ControlPanel::remove_port(const Path& path)
{
	bool was_first = false;
	for (vector<Control*>::iterator cg = _controls.begin(); cg != _controls.end(); ++cg) {
		if ((*cg)->port_model()->path() == path) {
			_control_box->remove(**cg);
			if (cg == _controls.begin())
				was_first = true;
			_controls.erase(cg);
			break;
		}
	}
}


/** Rename the control for the given port.
 */
/*
void
ControlPanel::rename_port(const Path& old_path, const Path& new_path)
{
	for (vector<Control*>::iterator cg = _controls.begin(); cg != _controls.end(); ++cg) {
		if ((*cg)->port_model()->path() == old_path) {
			(*cg)->set_name(new_path.name());
			return;
		}
	}
}
*/

#if 0
/** Enable the control for the given port.
 *
 * Used when all connections to port are un-made.
 */
void
ControlPanel::enable_port(const Path& path)
{
	for (vector<Control*>::iterator i = _controls.begin(); i != _controls.end(); ++i) {
		if ((*i)->port_model()->path() == path) {
			(*i)->enable();
			return;
		}
	}
}


/** Disable the control for the given port.
 *
 * Used when port is connected.
 */
void
ControlPanel::disable_port(const Path& path)
{
	for (vector<Control*>::iterator i = _controls.begin(); i != _controls.end(); ++i) {
		if ((*i)->port_model()->path() == path) {
			(*i)->disable();
			return;
		}
	}
}
#endif

/** Callback for Controls to notify this of a change.
 */
void
ControlPanel::value_changed(SharedPtr<PortModel> port, float val)
{
	if (_callback_enabled) {
	
		if (_all_voices_radio->get_active()) {
			App::instance().engine()->set_port_value_immediate(port->path(), Atom(val));
			port->value(val);
		} else {
			int voice = _voice_spinbutton->get_value_as_int() - 1;
			App::instance().engine()->set_voice_value_immediate(port->path(), voice, Atom(val));
			port->value(val);
		}

	}
}

void
ControlPanel::all_voices_selected()
{
	_voice_spinbutton->property_sensitive() = false;
}


void
ControlPanel::specific_voice_selected()
{
	_voice_spinbutton->property_sensitive() = true;
}


void
ControlPanel::polyphony_changed(uint32_t poly)
{
	_voice_spinbutton->set_range(0, poly - 1);
}


void
ControlPanel::polyphonic_changed(bool poly)
{
	if (poly)
		_voice_control_box->show();
	else
		_voice_control_box->hide();
}

	
} // namespace GUI
} // namespace Ingen
