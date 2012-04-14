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

#include "ingen/Interface.hpp"
#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/client/NodeModel.hpp"
#include "ingen/client/PortModel.hpp"
#include "ingen/client/PluginModel.hpp"
#include "App.hpp"
#include "ControlPanel.hpp"
#include "Controls.hpp"
#include "WidgetFactory.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace GUI {

ControlPanel::ControlPanel(BaseObjectType*                   cobject,
                           const Glib::RefPtr<Gtk::Builder>& xml)
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
ControlPanel::init(App& app, SharedPtr<const NodeModel> node, uint32_t poly)
{
	assert(node != NULL);
	assert(poly > 0);

	_app = &app;

	for (NodeModel::Ports::const_iterator i = node->ports().begin();
	     i != node->ports().end(); ++i) {
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
ControlPanel::add_port(SharedPtr<const PortModel> pm)
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
			WidgetFactory::get_widget_derived("toggle_control", tc);
			control = tc;
		} else if (pm->is_a(_app->uris().lv2_ControlPort)
		           || pm->is_a(_app->uris().lv2_CVPort)
		           || pm->supports(_app->uris().atom_Float)) {
			SliderControl* sc;
			WidgetFactory::get_widget_derived("control_strip", sc);
			control = sc;
		} else if (pm->supports(_app->uris().atom_String)) {
			StringControl* sc;
			WidgetFactory::get_widget_derived("string_control", sc);
			control = sc;
		}
	}

	if (control) {
		control->init(*_app, this, pm);

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
	for (vector<Control*>::iterator cg = _controls.begin(); cg != _controls.end(); ++cg) {
		if ((*cg)->port_model()->path() == path) {
			_control_box->remove(**cg);
			_controls.erase(cg);
			break;
		}
	}
}

/** Callback for Controls to notify this of a change.
 */
void
ControlPanel::value_changed_atom(SharedPtr<const PortModel> port,
                                 const Raul::Atom&          val)
{
	if (_callback_enabled) {
		_app->engine()->set_property(port->path(),
		                             _app->uris().ingen_value,
		                             val);
	}
}

} // namespace GUI
} // namespace Ingen
