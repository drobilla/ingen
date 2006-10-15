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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include <gtkmm.h>
#include <sigc++/sigc++.h>
#include <libglademm/xml.h>
#include <libglademm.h>
#include <vector>
#include <string>
#include <iostream>
#include <utility> // for pair<>
#include "ControlGroups.h"
#include "raul/Path.h"

using std::vector; using std::string; using std::pair;
using std::cerr; using std::cout; using std::endl;

namespace Ingen { namespace Client {
	class PortModel;
	class NodeModel;
} }
using namespace Ingen::Client;

namespace Ingenuity {


/** A group of controls for a node (or patch).
 *
 * Used by both NodeControlWindow and the main window (for patch controls).
 *
 * \ingroup Ingenuity
 */
class ControlPanel : public Gtk::HBox {
public:
	ControlPanel(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml);
	virtual ~ControlPanel();
	
	void init(SharedPtr<NodeModel> node, size_t poly);

	ControlGroup* find_port(const Path& path) const;

	void add_port(SharedPtr<PortModel> port);
	void remove_port(const Path& path);

	void enable_port(const Path& path);
	void disable_port(const Path& path);
	
	size_t        num_controls() const { return m_controls.size(); }
	pair<int,int> ideal_size()   const { return m_ideal_size; }
	
	// Callback for ControlGroup (FIXME: ugly)
	void value_changed(const Path& port_path, float val);

private:
	void all_voices_selected();
	void specific_voice_selected();
	void voice_selected();

	bool m_callback_enabled;
	
	pair<int,int> m_ideal_size;

	vector<ControlGroup*>    m_controls;
	Gtk::HBox*               m_control_box;
	Gtk::Box*                m_voice_control_box;
	Gtk::RadioButton*        m_all_voices_radio;
	Gtk::RadioButton*        m_specific_voice_radio;
	Gtk::SpinButton*         m_voice_spinbutton;
};


} // namespace Ingenuity

#endif // CONTROLPANEL_H
