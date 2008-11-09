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
#include "client/NodeModel.hpp"
#include "client/PluginModel.hpp"
#include "NodePropertiesWindow.hpp"

namespace Ingen {
namespace GUI {
using std::string;


NodePropertiesWindow::NodePropertiesWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml)
: Gtk::Window(cobject)
{
	glade_xml->get_widget("node_properties_path_label", _node_path_label);
	glade_xml->get_widget("node_properties_polyphonic_checkbutton", _node_polyphonic_toggle);
	glade_xml->get_widget("node_properties_plugin_type_label", _plugin_type_label);
	glade_xml->get_widget("node_properties_plugin_uri_label", _plugin_uri_label);
	glade_xml->get_widget("node_properties_plugin_name_label", _plugin_name_label);
}


/** Set the node this window is associated with.
 * This function MUST be called before using this object in any way.
 */
void
NodePropertiesWindow::set_node(SharedPtr<NodeModel> node_model)
{
	assert(node_model);
	
	_node_model = node_model;

	set_title(node_model->path() + " Properties");
	
	_node_path_label->set_text(node_model->path());
	_node_polyphonic_toggle->set_active(node_model->polyphonic());

	const PluginModel* pm = dynamic_cast<const PluginModel*>(node_model->plugin());
	if (pm) {
		_plugin_type_label->set_text(pm->type_uri());
		_plugin_uri_label->set_text(pm->uri());
		const Atom& name = pm->get_property("doap:name");
		if (name.is_valid())
			_plugin_name_label->set_text(pm->get_property("doap:name").get_string());
		else
			_plugin_name_label->set_text("(Unknown)");
	}
}


} // namespace GUI
} // namespace Ingen

