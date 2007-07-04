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

#include "interface/EngineInterface.h"
#include "client/PatchModel.h"
#include "client/NodeModel.h"
#include "client/PortModel.h"
#include "client/PluginModel.h"
#include "App.h"
#include "ControlPanel.h"
#include "ControlGroups.h"
#include "GladeFactory.h"

namespace Ingen {
namespace GUI {

	
ControlPanel::ControlPanel(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::HBox(cobject),
  _callback_enabled(true)
{
	xml->get_widget("control_panel_controls_box", _control_box);
	xml->get_widget("control_panel_voice_controls_box", _voice_control_box);
	xml->get_widget("control_panel_all_voices_radio", _all_voices_radio);
	xml->get_widget("control_panel_specific_voice_radio", _specific_voice_radio);
	xml->get_widget("control_panel_voice_spinbutton", _voice_spinbutton);
	
	_all_voices_radio->signal_toggled().connect(sigc::mem_fun(this, &ControlPanel::all_voices_selected));
	_specific_voice_radio->signal_toggled().connect(sigc::mem_fun(this, &ControlPanel::specific_voice_selected));
	_voice_spinbutton->signal_value_changed().connect(sigc::mem_fun(this, &ControlPanel::voice_selected));

	show_all();
}


ControlPanel::~ControlPanel()
{
	for (vector<ControlGroup*>::iterator i = _controls.begin(); i != _controls.end(); ++i)
		delete (*i);
}


void
ControlPanel::init(SharedPtr<NodeModel> node, size_t poly)
{
	assert(node != NULL);
	assert(poly > 0);
	
	if (poly > 1) {
		_voice_spinbutton->set_range(0, poly - 1);
	} else {
		remove(*_voice_control_box);
	}

	for (PortModelList::const_iterator i = node->ports().begin(); i != node->ports().end(); ++i) {
		add_port(*i);
	}
	
	_callback_enabled = true;
}


ControlGroup*
ControlPanel::find_port(const Path& path) const
{
	for (vector<ControlGroup*>::const_iterator cg = _controls.begin(); cg != _controls.end(); ++cg)
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
	if (pm->is_control() && pm->is_input()) {
		SliderControlGroup* cg = NULL;
#if 0
		if (pm->is_integer())
			cerr << "FIXME: integer\n";
			//cg = new IntegerControlGroup(this, pm);
		else if (pm->is_toggle())
			cerr << "FIXME: toggle\n";
			//cg = new ToggleControlGroup(this, pm);
		else {
#endif
			Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference("control_strip");
			xml->get_widget_derived("control_strip", cg);
			cg->init(this, pm);
#if 0
		}
#endif
	
		if (_controls.size() > 0)
			_control_box->pack_start(*Gtk::manage(new Gtk::HSeparator()), false, false, 4);
		
		_controls.push_back(cg);
		_control_box->pack_start(*cg, false, false, 0);

		cg->enable();
		/*if (pm->connected())
			cg->disable();
		else
			cg->enable();*/

		cg->show();

		cerr << "FIXME: control panel connected port tracking\n";
		//	pm->connection_sig.connect(bind(sigc::mem_fun(this, &ControlPanel::connection), pm))
		//	pm->disconnection_sig.connect(bind(sigc::mem_fun(this, &ControlPanel::disconnection), pm))
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
	for (vector<ControlGroup*>::iterator cg = _controls.begin(); cg != _controls.end(); ++cg) {
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
	for (vector<ControlGroup*>::iterator cg = _controls.begin(); cg != _controls.end(); ++cg) {
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
	for (vector<ControlGroup*>::iterator i = _controls.begin(); i != _controls.end(); ++i) {
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
	for (vector<ControlGroup*>::iterator i = _controls.begin(); i != _controls.end(); ++i) {
		if ((*i)->port_model()->path() == path) {
			(*i)->disable();
			return;
		}
	}
}
#endif

/** Callback for ControlGroups to notify this of a change.
 */
void
ControlPanel::value_changed(SharedPtr<PortModel> port, float val)
{
	if (_callback_enabled) {
		App::instance().engine()->disable_responses();

		/* Send the message, but set the client-side model's value to the new
		 * setting right away (so the value doesn't need to be echoed back) */
	
		if (_all_voices_radio->get_active()) {
			App::instance().engine()->set_port_value(port->path(), val);
			port->value(val);
		} else {
			int voice = _voice_spinbutton->get_value_as_int();
			App::instance().engine()->set_port_value(port->path(), voice, val);
			port->value(val);
		}

		App::instance().engine()->set_next_response_id(rand()); // FIXME: inefficient, probably not good
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
ControlPanel::voice_selected()
{
}

	
} // namespace GUI
} // namespace Ingen
