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

#include <cassert>
#include <string>
#include "interface/EngineInterface.hpp"
#include "client/NodeModel.hpp"
#include "client/PluginModel.hpp"
#include "App.hpp"
#include "Controls.hpp"
#include "PortPropertiesWindow.hpp"

using namespace std;

namespace Ingen {
namespace GUI {


PortPropertiesWindow::PortPropertiesWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Gtk::Window(cobject)
	, _enable_signal(false)
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
PortPropertiesWindow::present(SharedPtr<PortModel> pm)
{
	assert(pm);
	
	for (list<sigc::connection>::iterator i = _connections.begin(); i != _connections.end(); ++i)
		(*i).disconnect();

	_connections.clear();

	_port_model = pm;

	set_title(pm->path() + " Properties");
	
	float min = 0.0f, max = 1.0f;
	boost::shared_ptr<NodeModel> parent = PtrCast<NodeModel>(_port_model->parent());
	if (parent)
		parent->port_value_range(_port_model, min, max);

	_initial_min = min;
	_initial_max = max;

	_min_spinner->set_value(min);
	_connections.push_back(_min_spinner->signal_value_changed().connect(
				sigc::mem_fun(*this, &PortPropertiesWindow::min_changed)));
	
	_max_spinner->set_value(max);
	_connections.push_back(_max_spinner->signal_value_changed().connect(
				sigc::mem_fun(*this, &PortPropertiesWindow::max_changed)));
	
	_connections.push_back(pm->signal_variable.connect(
			sigc::mem_fun(this, &PortPropertiesWindow::variable_change)));

	_enable_signal = true;

	Gtk::Window::present();
}


void
PortPropertiesWindow::variable_change(const string& key, const Atom& value)
{
	_enable_signal = false;

	if ( (key == "lv2:minimum") && value.type() == Atom::FLOAT)
		_min_spinner->set_value(value.get_float());
	else if ( (key == "lv2:maximum") && value.type() == Atom::FLOAT)
		_max_spinner->set_value(value.get_float());
	
	_enable_signal = true;
}


void
PortPropertiesWindow::min_changed()
{
	const float min = _min_spinner->get_value();
	float max       = _max_spinner->get_value();
	
	if (max <= min) {
		max = min + 1.0;
		_max_spinner->set_value(max);
	}

	if (_enable_signal)
		App::instance().engine()->set_variable(_port_model->path(), "lv2:minimum", min);
}


void
PortPropertiesWindow::max_changed()
{
	float min       = _min_spinner->get_value();
	const float max = _max_spinner->get_value();
	
	if (min >= max) {
		min = max - 1.0;
		_min_spinner->set_value(min);
	}

	if (_enable_signal)
		App::instance().engine()->set_variable(_port_model->path(), "lv2:maximum", max);
}


void
PortPropertiesWindow::cancel()
{
	App::instance().engine()->set_variable(_port_model->path(), "lv2:minimum", _initial_min);
	App::instance().engine()->set_variable(_port_model->path(), "lv2:maximum", _initial_max);
	hide();
}


void
PortPropertiesWindow::ok()
{
	hide();
}


} // namespace GUI
} // namespace Ingen

