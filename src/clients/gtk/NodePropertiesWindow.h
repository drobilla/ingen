/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef NODEPROPERTIESWINDOW_H
#define NODEPROPERTIESWINDOW_H

#include <gtkmm.h>
#include <libglademm.h>

namespace LibOmClient { class NodeModel; }
using namespace LibOmClient;

namespace OmGtk {


/** 'New Patch' Window.
 *
 * Loaded by libglade as a derived object.
 *
 * \ingroup OmGtk
 */
class NodePropertiesWindow : public Gtk::Window
{
public:
	NodePropertiesWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade);

	void set_node(NodeModel* node_model);

private:
	
	NodeModel*        m_node_model;

	Gtk::Label*       m_node_path_label;
	Gtk::CheckButton* m_node_polyphonic_toggle;
	Gtk::Label*       m_plugin_type_label;
	Gtk::Label*       m_plugin_uri_label;
	Gtk::Label*       m_plugin_name_label;
};

} // namespace OmGtk

#endif // NODEPROPERTIESWINDOW_H
