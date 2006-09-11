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

#ifndef PATCHCONTROLLER_H
#define PATCHCONTROLLER_H

#include <string>
#include <gtkmm.h>
#include "util/CountedPtr.h"
#include "NodeController.h"
#include "PatchModel.h"

namespace Ingen { namespace Client {
	class PatchModel;
	class NodeModel;
	class PortModel;
	class ControlModel;
	class ConnectionModel;
} }

using std::string;
using namespace Ingen::Client;

namespace Ingenuity {
	
class PatchWindow;
class SubpatchModule;
class Controller;
class OmFlowCanvas;
class NodeControlWindow;
class PatchPropertiesWindow;
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
 * \ingroup Ingenuity
 */
class PatchController : public NodeController
{
public:
	virtual ~PatchController();
	
	/*
	virtual void add_to_store();
	virtual void remove_from_store();
	*/

	virtual void metadata_update(const string& key, const string& value);
	
	//virtual void add_port(CountedPtr<PortModel> pm);
	//virtual void remove_port(const Path& path, bool resize_module);

	void connection(CountedPtr<ConnectionModel> cm);
	void disconnection(const Path& src_port_path, const Path& dst_port_path);
	//void clear();

	void show_control_window();
	void show_properties_window();
	void show_patch_window();
	
	void claim_patch_view();
	
	void create_module(OmFlowCanvas* canvas);

	CountedPtr<PatchView> get_view();
	PatchWindow*       window() const          { return m_window; }
	void               window(PatchWindow* pw) { m_window = pw; }
	
	void set_path(const Path& new_path);

	//void enable();
	//void disable();
	
	const CountedPtr<PatchModel> patch_model() const { return m_patch_model; }
	
private:
	friend class ControllerFactory;
	PatchController(CountedPtr<PatchModel> model);
	
	void add_node(CountedPtr<NodeModel> object);
	
	PatchPropertiesWindow* m_properties_window;

	// FIXME: remove these
	PatchWindow*          m_window;     ///< Patch Window currently showing m_patch_view
	CountedPtr<PatchView> m_patch_view; ///< View (canvas) of this patch

	CountedPtr<PatchModel> m_patch_model;

	/** Invisible bin used to store patch view when not shown by a patch window */
	Gtk::Alignment m_patch_view_bin;

	// Coordinates for next added plugin (used by canvas menu)
	// 0 means "not set", ie guess at the best location
	int m_module_x;
	int m_module_y;
};


} // namespace Ingenuity

#endif // PATCHCONTROLLER_H
