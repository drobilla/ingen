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
 * You should have received a copy of the GNU General Public License alongCont
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "App.h"
#include "ModelEngineInterface.h"
#include "ControlPanel.h"
#include "PatchModel.h"
#include "NodeModel.h"
#include "PortModel.h"
#include "ControlGroups.h"

namespace Ingenuity {

	
ControlPanel::ControlPanel(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::VBox(cobject),
  m_callback_enabled(true)
{
	xml->get_widget("control_panel_controls_box", m_control_box);
	xml->get_widget("control_panel_voice_controls_box", m_voice_control_box);
	xml->get_widget("control_panel_all_voices_radio", m_all_voices_radio);
	xml->get_widget("control_panel_specific_voice_radio", m_specific_voice_radio);
	xml->get_widget("control_panel_voice_spinbutton", m_voice_spinbutton);
	
	m_all_voices_radio->signal_toggled().connect(sigc::mem_fun(this, &ControlPanel::all_voices_selected));
	m_specific_voice_radio->signal_toggled().connect(sigc::mem_fun(this, &ControlPanel::specific_voice_selected));
	m_voice_spinbutton->signal_value_changed().connect(sigc::mem_fun(this, &ControlPanel::voice_selected));

	show_all();
}


ControlPanel::~ControlPanel()
{
	for (vector<ControlGroup*>::iterator i = m_controls.begin(); i != m_controls.end(); ++i)
		delete (*i);
}


void
ControlPanel::init(CountedPtr<NodeModel> node, size_t poly)
{
	assert(node != NULL);
	assert(poly > 0);
	
	if (poly > 1) {
		m_voice_spinbutton->set_range(0, poly - 1);
	} else {
		remove(*m_voice_control_box);
	}

	for (PortModelList::const_iterator i = node->ports().begin(); i != node->ports().end(); ++i) {
		add_port(*i);
	}
	
	m_callback_enabled = true;
}


ControlGroup*
ControlPanel::find_port(const Path& path) const
{
	for (vector<ControlGroup*>::const_iterator cg = m_controls.begin(); cg != m_controls.end(); ++cg)
		if ((*cg)->port_model()->path() == path)
			return (*cg);

	return NULL;
}


/** Add a control to the panel for the given port.
 */
void
ControlPanel::add_port(CountedPtr<PortModel> pm)
{
	assert(pm);
	
	// Already have port, don't add another
	if (find_port(pm->path()) != NULL)
		return;
	
	// Add port
	if (pm->is_control() && pm->is_input()) {
		bool separator = (m_controls.size() > 0);
		ControlGroup* cg = NULL;
		if (pm->is_integer())
			cg = new IntegerControlGroup(this, pm, separator);
		else if (pm->is_toggle())
			cg = new ToggleControlGroup(this, pm, separator);
		else
			cg = new SliderControlGroup(this, pm, separator);
	
		m_controls.push_back(cg);
		m_control_box->pack_start(*cg, false, false, 0);

		if (pm->connected())
			cg->disable();
		else
			cg->enable();

		cerr << "FIXME: Control panel add port\n";
		//	pm->connection_sig.connect(bind(sigc::mem_fun(this, &ControlPanel::connection), pm))
		//	pm->disconnection_sig.connect(bind(sigc::mem_fun(this, &ControlPanel::disconnection), pm))
	}

	Gtk::Requisition controls_size;
	m_control_box->size_request(controls_size);
	m_ideal_size.first = controls_size.width;
	m_ideal_size.second = controls_size.height;
	
	Gtk::Requisition voice_size;
	m_voice_control_box->size_request(voice_size);
	m_ideal_size.first += voice_size.width;
	m_ideal_size.second += voice_size.height;

	//cerr << "Setting ideal size to " << m_ideal_size.first
	//	<< " x " << m_ideal_size.second << endl;
}


/** Remove the control for the given port.
 */
void
ControlPanel::remove_port(const Path& path)
{
	bool was_first = false;
	for (vector<ControlGroup*>::iterator cg = m_controls.begin(); cg != m_controls.end(); ++cg) {
		if ((*cg)->port_model()->path() == path) {
			m_control_box->remove(**cg);
			if (cg == m_controls.begin())
				was_first = true;
			m_controls.erase(cg);
			break;
		}
	}
	
	if (was_first && m_controls.size() > 0)
		(*m_controls.begin())->remove_separator();		
}


/** Rename the control for the given port.
 */
/*
void
ControlPanel::rename_port(const Path& old_path, const Path& new_path)
{
	for (vector<ControlGroup*>::iterator cg = m_controls.begin(); cg != m_controls.end(); ++cg) {
		if ((*cg)->port_model()->path() == old_path) {
			(*cg)->set_name(new_path.name());
			return;
		}
	}
}
*/

/** Enable the control for the given port.
 *
 * Used when all connections to port are un-made.
 */
void
ControlPanel::enable_port(const Path& path)
{
	for (vector<ControlGroup*>::iterator i = m_controls.begin(); i != m_controls.end(); ++i) {
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
	for (vector<ControlGroup*>::iterator i = m_controls.begin(); i != m_controls.end(); ++i) {
		if ((*i)->port_model()->path() == path) {
			(*i)->disable();
			return;
		}
	}
}


/** Callback for ControlGroups to notify this of a change.
 */
void
ControlPanel::value_changed(const Path& port_path, float val)
{
	if (m_callback_enabled) {
		if (m_all_voices_radio->get_active()) {
			App::instance().engine()->set_port_value(port_path, val);
		} else {
			int voice = m_voice_spinbutton->get_value_as_int();
			App::instance().engine()->set_port_value(port_path, voice, val);
		}
	}
}

/*
void
ControlPanel::set_range_min(const Path& port_path, float val)
{
	bool found = false;
	for (vector<ControlGroup*>::iterator i = m_controls.begin(); i != m_controls.end(); ++i) {
		if ((*i)->port_model()->path() == port_path) {
			found = true;
			(*i)->set_min(val);
		}
	}
	if (found == false)
		cerr << "[ControlPanel::set_range_min] Unable to find control " << port_path << endl;
}


void
ControlPanel::set_range_max(const Path& port_path, float val)
{
	bool found = false;
	for (vector<ControlGroup*>::iterator i = m_controls.begin(); i != m_controls.end(); ++i) {
		if ((*i)->port_model()->path() == port_path) {
			found = true;
			(*i)->set_max(val);
		}
	}
	if (found == false)
		cerr << "[ControlPanel::set_range_max] Unable to find control " << port_path << endl;
}
*/

void
ControlPanel::all_voices_selected()
{
	m_voice_spinbutton->property_sensitive() = false;
}


void
ControlPanel::specific_voice_selected()
{
	m_voice_spinbutton->property_sensitive() = true;
}


void
ControlPanel::voice_selected()
{
}

	
} // namespace Ingenuity
