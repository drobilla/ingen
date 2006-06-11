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

#ifndef NODECONTROLLER_H
#define NODECONTROLLER_H

#include <string>
#include <gtkmm.h>
#include "util/Path.h"
#include "GtkObjectController.h"
#include "NodeModel.h"

using std::string;
using Om::Path;
using namespace LibOmClient;

template <class T> class CountedPtr;

namespace LibOmClient {
	class MetadataModel;
	class PortModel;
}

namespace OmGtk {

class Controller;
class OmModule;
class NodeControlWindow;
class NodePropertiesWindow;
class PortController;
class OmFlowCanvas;

/** Controller for a Node.
 *
 * \ingroup OmGtk
 */
class NodeController : public GtkObjectController
{
public:
	NodeController(CountedPtr<NodeModel> model);
	virtual ~NodeController();
	
	virtual void destroy();

	/*virtual void add_to_store();
	virtual void remove_from_store();*/

	virtual void metadata_update(const string& key, const string& value);
	
	virtual void create_module(OmFlowCanvas* canvas);

	void set_path(const Path& new_path);
	
	virtual void add_port(PortModel* pm);
	virtual void remove_port(const Path& path, bool resize_module) {}

	virtual void program_add(int bank, int program, const string& name) {}
	virtual void program_remove(int bank, int program) {}

	OmModule* module()                { return m_module; }
	
	void bridge_port(PortController* port) { m_bridge_port = port; }
	PortController* as_port()              { return m_bridge_port; }
	
	CountedPtr<NodeModel> node_model() { return CountedPtr<NodeModel>((NodeModel*)m_model.get()); }
	
	NodeControlWindow* control_window()        { return m_control_window; }
	void control_window(NodeControlWindow* cw) { m_control_window = cw; }
	
	virtual void show_control_window();
	void         show_rename_window();
	void         show_properties_window();
	
	bool has_control_inputs();
	
	virtual void show_menu(GdkEventButton* event)
	{ m_menu.popup(event->button, event->time); }
	
	virtual void enable_controls_menuitem();
	virtual void disable_controls_menuitem();

protected:
	void create_all_ports();

	void on_menu_destroy();
	void on_menu_clone();
	void on_menu_learn();
	void on_menu_disconnect_all();

	Gtk::Menu                   m_menu;
	Glib::RefPtr<Gtk::MenuItem> m_controls_menuitem;
	
	OmModule* m_module; ///< View (module on a patch canvas)
	
	NodeControlWindow*    m_control_window;
	NodePropertiesWindow* m_properties_window;
	PortController*       m_bridge_port;
};


} // namespace OmGtk

#endif // NODECONTROLLER_H
