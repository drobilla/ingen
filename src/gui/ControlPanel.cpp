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
 * You should have received a copy of the GNU General Public License alongCont
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "interface/EngineInterface.hpp"
#include "interface/PortType.hpp"
#include "shared/LV2URIMap.hpp"
#include "client/NodeModel.hpp"
#include "client/PortModel.hpp"
#include "client/PluginModel.hpp"
#include "App.hpp"
#include "ControlPanel.hpp"
#include "Controls.hpp"
#include "GladeFactory.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace GUI {


ControlPanel::ControlPanel(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Gtk::HBox(cobject)
	, _callback_enabled(true)
{
	xml->get_widget("control_panel_controls_box", _control_box);

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

	for (NodeModel::Ports::const_iterator i = node->ports().begin(); i != node->ports().end(); ++i) {
		add_port(*i);
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

	Control* control = NULL;

	// Add port
	if (pm->is_input()) {
		if (pm->is_toggle()) {
			ToggleControl* tc;
			Glib::RefPtr<Gnome::Glade::Xml> xml
					= GladeFactory::new_glade_reference("toggle_control");
			xml->get_widget_derived("toggle_control", tc);
			control = tc;
		} else if (pm->is_a(Shared::PortType::CONTROL)
				|| pm->supports(App::instance().uris().object_class_float32)) {
			SliderControl* sc;
			Glib::RefPtr<Gnome::Glade::Xml> xml
					= GladeFactory::new_glade_reference("control_strip");
			xml->get_widget_derived("control_strip", sc);
			control = sc;
		} else if (pm->supports(App::instance().uris().object_class_string)) {
			StringControl* sc;
			Glib::RefPtr<Gnome::Glade::Xml> xml
					= GladeFactory::new_glade_reference("string_control");
			xml->get_widget_derived("string_control", sc);
			control = sc;
		}
	}

	if (control) {
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


/** Callback for Controls to notify this of a change.
 */
void
ControlPanel::value_changed_atom(SharedPtr<PortModel> port, const Raul::Atom& val)
{
	if (_callback_enabled) {
		App::instance().engine()->set_property(port->path(),
				App::instance().uris().ingen_value,
				val);
		port->value(val);
	}
}


} // namespace GUI
} // namespace Ingen
