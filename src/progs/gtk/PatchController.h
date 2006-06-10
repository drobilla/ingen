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

#ifndef PATCHCONTROLLER_H
#define PATCHCONTROLLER_H

#include <string>
#include <gtkmm.h>
#include "NodeController.h"

namespace LibOmClient {
class PatchModel;
class NodeModel;
class PortModel;
class ControlModel;
class ConnectionModel;
}

using std::string;
using namespace LibOmClient;

namespace OmGtk {
	
class PatchWindow;
class SubpatchModule;
class Controller;
class OmFlowCanvas;
class NodeControlWindow;
class ControlPanel;
class PatchView;
class NodeController;


/** Controller for a patch.
 *
 * A patch is different from a port or node because there are two
 * representations in the Gtk client - the window and the module in the parent
 * patch (if applicable).  So, this is a master class that contains both of
 * those representations, and acts as the recipient of all patch related
 * events (being the controller).
 * 
 * \ingroup OmGtk
 */
class PatchController : public NodeController
{
public:
	PatchController(PatchModel* model);
	virtual ~PatchController();
	
	virtual void add_to_store();
	virtual void remove_from_store();

	virtual void destroy();

	virtual void metadata_update(const string& key, const string& value);
	
	void add_node(NodeModel* nm);
	void remove_node(const string& name);
	
	virtual void add_port(PortModel* pm);
	virtual void remove_port(const Path& path, bool resize_module);

	void connection(ConnectionModel* const cm);
	void disconnection(const Path& src_port_path, const Path& dst_port_path);
	void clear();

	void add_subpatch(PatchController* patch);

	void get_new_module_location(int& x, int& y);

	void show_control_window();
	void show_patch_window();
	
	void claim_patch_view();
	
	void create_module(OmFlowCanvas* canvas);
	void create_view();

	PatchView*         view() const            { return m_patch_view; }
	PatchWindow*       window() const          { return m_window; }
	void               window(PatchWindow* pw) { m_window = pw; }
	
	inline string        name() const { return m_model->name(); }
	inline const string& path() const { return m_model->path(); }
	
	void set_path(const Path& new_path);

	void enable();
	void disable();
	
	PatchModel* patch_model() const { return (PatchModel*)m_model; }
	
	void enable_controls_menuitem();
	void disable_controls_menuitem();

private:
	void create_connection(const ConnectionModel* cm);

	PatchWindow* m_window;     ///< Window currently showing this patch
	PatchView*   m_patch_view; ///< View (canvas) of this patch

	/** Invisible bin used to store patch view when not shown by a patch window */
	Gtk::Alignment m_patch_view_bin;

	// Coordinates for next added plugin (used by canvas menu)
	// 0 means "not set", ie guess at the best location
	int m_module_x;
	int m_module_y;
};


} // namespace OmGtk

#endif // PATCHCONTROLLER_H
