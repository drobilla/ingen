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

#include <cassert>
#include <string>

#include "ingen/Interface.hpp"
#include "ingen/client/NodeModel.hpp"
#include "ingen/client/PluginModel.hpp"

#include "App.hpp"
#include "PortPropertiesWindow.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
using namespace Client;
namespace GUI {

PortPropertiesWindow::PortPropertiesWindow(BaseObjectType*                   cobject,
                                           const Glib::RefPtr<Gtk::Builder>& xml)
	: Window(cobject)
	, _initial_min(0.0f)
	, _initial_max(1.0f)
{
	xml->get_widget("port_properties_min_spinner", _min_spinner);
	xml->get_widget("port_properties_max_spinner", _max_spinner);
	xml->get_widget("port_properties_cancel_button", _cancel_button);
	xml->get_widget("port_properties_ok_button", _ok_button);

	_cancel_button->signal_clicked().connect(sigc::mem_fun(this,
				&PortPropertiesWindow::cancel));

	_ok_button->signal_clicked().connect(sigc::mem_fun(this,
				&PortPropertiesWindow::ok));
}

/** Set the port this window is associated with.
 * This function MUST be called before using this object in any way.
 */
void
PortPropertiesWindow::present(SharedPtr<const PortModel> pm)
{
	assert(pm);

	for (list<sigc::connection>::iterator i = _connections.begin(); i != _connections.end(); ++i)
		(*i).disconnect();

	_connections.clear();

	_port_model = pm;

	set_title(pm->path().chop_scheme() + " Properties - Ingen");

	float min = 0.0f, max = 1.0f;
	boost::shared_ptr<NodeModel> parent = PtrCast<NodeModel>(_port_model->parent());
	if (parent)
		parent->port_value_range(_port_model, min, max,
		                         _app->sample_rate());

	_initial_min = min;
	_initial_max = max;

	_min_spinner->set_value(min);
	_connections.push_back(
		_min_spinner->signal_value_changed().connect(
			sigc::mem_fun(*this, &PortPropertiesWindow::min_changed)));

	_max_spinner->set_value(max);
	_connections.push_back(
		_max_spinner->signal_value_changed().connect(
			sigc::mem_fun(*this, &PortPropertiesWindow::max_changed)));

	_connections.push_back(
		pm->signal_property().connect(
			sigc::mem_fun(this, &PortPropertiesWindow::property_changed)));

	Gtk::Window::present();
}

void
PortPropertiesWindow::property_changed(const URI& key, const Atom& value)
{
	const Shared::URIs& uris = _app->uris();

	if (value.type() == uris.forge.Float) {
		if (key == uris.lv2_minimum)
			_min_spinner->set_value(value.get_float());
		else if (key == uris.lv2_maximum)
			_max_spinner->set_value(value.get_float());
	}
}

void
PortPropertiesWindow::min_changed()
{
	const float val = _port_model->value().get_float();
	float       min = _min_spinner->get_value();
	float       max = _max_spinner->get_value();

	if (min > val) {
		_min_spinner->set_value(val);
		return; // avoid recursion
	}

	if (max <= min) {
		max = min + 1.0;
		_max_spinner->set_value(max);
	}
}

void
PortPropertiesWindow::max_changed()
{
	const float val = _port_model->value().get_float();
	float       min = _min_spinner->get_value();
	float       max = _max_spinner->get_value();

	if (max < val) {
		_max_spinner->set_value(val);
		return; // avoid recursion
	}

	if (min >= max) {
		min = max - 1.0;
		_min_spinner->set_value(min);
	}
}

void
PortPropertiesWindow::cancel()
{
	hide();
}

void
PortPropertiesWindow::ok()
{
	const Shared::URIs& uris = _app->uris();
	Resource::Properties props;
	props.insert(
		make_pair(uris.lv2_minimum,
		          _app->forge().make(float(_min_spinner->get_value()))));
	props.insert(
		make_pair(uris.lv2_maximum,
		          _app->forge().make(float(_max_spinner->get_value()))));
	_app->engine()->put(_port_model->path(), props);
	hide();
}

} // namespace GUI
} // namespace Ingen

