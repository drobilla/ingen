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
#include "util/Path.h"
#include "PortController.h"

using std::vector; using std::string; using std::pair;
using std::cerr; using std::cout; using std::endl;

namespace LibOmClient {
class PortModel;
class NodeModel;
}
using namespace LibOmClient;
using Om::Path;

namespace OmGtk {

class NodeController;
class PortController;


/** A group of controls for a node (or patch).
 *
 * Used by both NodeControlWindow and the main window (for patch controls).
 *
 * \ingroup OmGtk
 */
class ControlPanel : public Gtk::VBox {
public:
	ControlPanel(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml);
	virtual ~ControlPanel();
	
	void init(NodeController* node, size_t poly);

	ControlGroup* find_port(const Path& path) const;

	void add_port(PortController* port);
	void remove_port(const Path& path);

	void rename_port(const Path& old_path, const Path& new_path);

	void enable_port(const Path& path);
	void disable_port(const Path& path);
	
	size_t        num_controls() const { return m_controls.size(); }
	pair<int,int> ideal_size()   const { return m_ideal_size; }
	
	// Callback for ControlGroup (FIXME: ugly)
	void value_changed(const Path& port_path, float val);

	//inline void set_control(const Path& port_path, float value);
	//void set_range_min(const Path& port_path, float value);
	//void set_range_max(const Path& port_path, float value);

private:
	void all_voices_selected();
	void specific_voice_selected();
	void voice_selected();

	bool m_callback_enabled;
	
	pair<int,int> m_ideal_size;

	vector<ControlGroup*>    m_controls;
	Gtk::VBox*               m_control_box;
	Gtk::Box*                m_voice_control_box;
	Gtk::RadioButton*        m_all_voices_radio;
	Gtk::RadioButton*        m_specific_voice_radio;
	Gtk::SpinButton*         m_voice_spinbutton;
};


/** Set a port on this panel to a certain value.
 *
 * Profiling has shown this is performance critical.  Needs to be made
 * faster.
 */
/*
inline void
ControlPanel::set_control(const Path& port_path, const float val)
{
	// FIXME: double lookup, ports should just have a pointer directly to
	// their control group
	
	m_callback_enabled = false;
	ControlGroup* cg   = NULL;
	
	for (vector<ControlGroup*>::iterator i = m_controls.begin(); i != m_controls.end(); ++i) {
		cg = (*i);
		if (cg->port_model()->path() == port_path) {
			cg->set_value(val);
			m_callback_enabled = true;
			return;
		}
	}
	
	cerr << "[ControlPanel::set_control] Unable to find control " << port_path << endl;
	m_callback_enabled = true;
}
*/

} // namespace OmGtk

#endif // CONTROLPANEL_H
