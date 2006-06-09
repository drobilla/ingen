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

#include "config.h"
#include "PatchController.h"
#include <cassert>
#include <cstdlib>
#include "GladeFactory.h"
#include "Configuration.h"
#include "util/Path.h"
#include "ControlPanel.h"
#include "ConnectionModel.h"
#include "OmFlowCanvas.h"
#include "PatchView.h"
#include "flowcanvas/Module.h"
#include "PluginModel.h"
#include "Controller.h"
#include "SubpatchModule.h"
#include "DSSIModule.h"
#include "PatchWindow.h"
#include "NodeModel.h"
#include "OmModule.h"
#include "OmPort.h"
#include "ControlModel.h"
#include "NodeControlWindow.h"
#include "NodeController.h"
#include "PortController.h"
#include "App.h"
#include "PatchTreeWindow.h"
#include "DSSIController.h"
#include "PatchModel.h"
#include "Store.h"

using std::cerr; using std::cout; using std::endl;
using Om::Path;
using namespace LibOmClient;

namespace OmGtk {


PatchController::PatchController(PatchModel* model)
: NodeController(model),
  m_window(NULL),
  m_patch_view(NULL),
  m_module_x(0),
  m_module_y(0)
{
	assert(model->path().length() > 0);
	assert(model->parent() == NULL);
	assert(model->controller() == this); // NodeController() does this

	if (model->path() != "/") {
		PatchController* parent = Store::instance().patch(model->path().parent());
		if (parent != NULL)
			parent->add_subpatch(this);
		else
			cerr << "[PatchController] " << path() << " ERROR: Parent not found." << endl;
	}
}


PatchController::~PatchController()
{
	if (m_patch_view != NULL) {
		claim_patch_view();
		m_patch_view->hide();
		delete m_patch_view;
		m_patch_view = NULL;
	}
	
	if (m_control_window != NULL) {
		m_control_window->hide();
		delete m_control_window;
		m_control_window = NULL;
	}

	if (m_window != NULL) {
		m_window->hide();
		delete m_window;
		m_window = NULL;
	}
}


void
PatchController::add_to_store()
{
	Store::instance().add_object(this);
}


void
PatchController::remove_from_store()
{
	Store::instance().remove_object(this);
}


void
PatchController::clear()
{
	// Destroy model
	// Destroying nodes removes models from patch model, which invalidates any
	// iterator to nodes, so avoid the iterator problem by doing it this way:
	const NodeModelMap& nodes = patch_model()->nodes();
	size_t remaining = nodes.size();
	
	while (remaining > 0) {
		NodeController* const nc = (NodeController*)(*nodes.begin()).second->controller();
		assert(nc != NULL);
		nc->destroy();
		assert(nodes.size() == remaining - 1);
		--remaining;
	}
	assert(nodes.empty());

	patch_model()->clear();
	
	if (m_patch_view != NULL) {
		assert(m_patch_view->canvas() != NULL);
		m_patch_view->canvas()->destroy();
	}
}


void
PatchController::destroy()
{
	// Destroying nodes removes models from patch model, which invalidates any
	// iterator to nodes, so avoid the iterator problem by doing it this way:
	const NodeModelMap& nodes = patch_model()->nodes();
	size_t remaining = nodes.size();
	
	while (remaining > 0) {
		NodeController* const nc = (NodeController*)
			(*nodes.begin()).second->controller();
		assert(nc != NULL);
		nc->destroy();
		assert(nodes.size() == remaining - 1);
		--remaining;
	}
	assert(nodes.empty());

	//App::instance().remove_patch(this);
	App::instance().patch_tree()->remove_patch(path());
	
	// Delete all children models
	//patch_model()->clear();

	// Remove self from object store
	Store::instance().remove_object(this);
	
	// Delete self from parent (this will delete model)
	if (patch_model()->parent() != NULL) {
		PatchController* const parent = (PatchController*)patch_model()->parent()->controller();
		assert(parent != NULL);
		parent->remove_node(name());
	} else {
		delete m_model;
	}
}


void
PatchController::metadata_update(const string& key, const string& value)
{
	NodeController::metadata_update(key, value);
	
	if (key == "filename")
		patch_model()->filename(value);
}


void
PatchController::set_path(const Path& new_path)
{
	assert(m_model != NULL);
	Path old_path = path();
	
	// Rename nodes
	for (NodeModelMap::const_iterator i = patch_model()->nodes().begin();
			i != patch_model()->nodes().end(); ++i) {
		const NodeModel* const nm = (*i).second;
		assert(nm != NULL);
		NodeController* const nc = ((NodeController*)nm->controller());
		assert(nc != NULL);
		nc->set_path(new_path.base_path() + nc->node_model()->name());
	}
	
#ifdef DEBUG
	// Be sure ports were renamed by their bridge nodes
	for (list<PortModel*>::const_iterator i = node_model()->ports().begin();
			i != node_model()->ports().end(); ++i) {
		GtkObjectController* const pc = (GtkObjectController*)((*i)->controller());
		assert(pc != NULL);
		assert(pc->path().parent()== new_path);
	}
#endif
	
	App::instance().patch_tree()->patch_renamed(old_path, new_path);
	
	if (m_window != NULL)
		m_window->patch_renamed(new_path);

	if (m_control_window != NULL)
		m_control_window->set_title(new_path + " Controls");

	if (m_module != NULL) {
		assert(m_module->canvas() != NULL);
		m_module->canvas()->rename_module(old_path.name(), new_path.name());
		assert(m_module->name() == new_path.name());
	}
	
	PatchController* parent = dynamic_cast<PatchController*>(
		patch_model()->parent()->controller());
	
	if (parent != NULL && parent->window() != NULL)
		parent->window()->node_renamed(old_path, new_path);

	remove_from_store();
	GtkObjectController::set_path(new_path);
	add_to_store();
	
	if (old_path.name() != new_path.name())
		parent->patch_model()->rename_node(old_path, new_path);
}


void
PatchController::enable()
{
	if (m_patch_view != NULL)
		m_patch_view->enabled(true);
	
	patch_model()->enabled(true);
	
	App::instance().patch_tree()->patch_enabled(m_model->path());
}


void
PatchController::disable()
{
	if (m_patch_view != NULL)
		m_patch_view->enabled(false);
	
	patch_model()->enabled(false);
	
	App::instance().patch_tree()->patch_disabled(m_model->path());
}


void
PatchController::create_module(OmFlowCanvas* canvas)
{
	//cerr << "Creating patch module " << m_model->path() << endl;

	assert(canvas != NULL);
	assert(m_module == NULL);
	
	m_module = new SubpatchModule(canvas, this);


	m_menu.remove(m_menu.items()[4]);

	// Add navigation menu items
	Gtk::Menu::MenuList& items = m_menu.items();
	items.push_front(Gtk::Menu_Helpers::SeparatorElem());
	items.push_front(Gtk::Menu_Helpers::MenuElem("Browse to Patch",
		sigc::mem_fun((SubpatchModule*)m_module, &SubpatchModule::browse_to_patch)));
	items.push_front(Gtk::Menu_Helpers::MenuElem("Open Patch in New Window",
		sigc::mem_fun(this, &PatchController::show_patch_window)));
	
	create_all_ports();

	m_module->move_to(node_model()->x(), node_model()->y());
	m_module->store_location();
}


void
PatchController::create_view()
{
	assert(m_patch_view == NULL);

	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference();

	xml->get_widget_derived("patch_view_vbox", m_patch_view);
	assert(m_patch_view != NULL);
	m_patch_view->patch_controller(this);
	assert(m_patch_view->canvas() != NULL);

	// Create modules for nodes
	for (NodeModelMap::const_iterator i = patch_model()->nodes().begin();
			i != patch_model()->nodes().end(); ++i) {

		NodeModel* const nm = (*i).second;

		string val = nm->get_metadata("module-x");
		if (val != "")
			nm->x(atof(val.c_str()));
		val = nm->get_metadata("module-y");
		if (val != "")
			nm->y(atof(val.c_str()));

		/* Set sane default coordinates if not set already yet */
		if (nm->x() == 0.0f && nm->y() == 0.0f) {
			int x, y;
			get_new_module_location(x, y);
			nm->x(x);
			nm->y(y);
		}

		NodeController* nc = ((NodeController*)nm->controller());
		assert(nc != NULL);
		if (nc->module() == NULL);
			nc->create_module(m_patch_view->canvas());
		assert(nc->module() != NULL);
		m_patch_view->canvas()->add_module(nc->module());
	}

	// Create connections
	for (list<ConnectionModel*>::const_iterator i = patch_model()->connections().begin();
			i != patch_model()->connections().end(); ++i) {
		create_connection(*i);
	}

	// Set run checkbox
	m_patch_view->enabled(patch_model()->enabled());
}


/** Create a connection in the view (canvas).
 */
void
PatchController::create_connection(const ConnectionModel* cm)
{
	m_patch_view->canvas()->add_connection(
			cm->src_port_path().parent().name(),
			cm->src_port_path().name(),
			cm->dst_port_path().parent().name(),
			cm->dst_port_path().name());

	// Disable control slider from destination node control window
	
	PortController* p = Store::instance().port(cm->dst_port_path());
	assert(p != NULL);

	if (p->control_panel() != NULL)
		p->control_panel()->disable_port(p->path());
	// FIXME: don't use canvas as a model (search object store)
	/*OmModule* m = (OmModule*)m_patch_view->canvas()->find_module(
			cm->dst_port_path().parent().name());

	if (m != NULL) {
		OmPort* p = m->port(cm->dst_port_path().name());
		if (p != NULL && p->connections().size() == 1) {
			p->model()->connected(true);
			assert(m->node_model()->controller() != NULL);
			NodeControlWindow* cw = (((NodeController*)
				m->node_model()->controller())->control_window());
			if (cw != NULL)
				cw->control_panel()->disable_port(cm->dst_port_path());
		}
	}*/
}


/** Add a subpatch to this patch.
 */
void
PatchController::add_subpatch(PatchController* patch)
{
	assert(patch != NULL);
	assert(patch->patch_model() != NULL);
	assert(patch->patch_model()->parent() == NULL);

	/*if (pm->x() == 0 && pm->y() == 0) {
		int x, y;
		parent_pc->get_new_module_location(x, y);
		pm->x(x);
		pm->y(y);
	}*/
	
	patch_model()->add_node(patch->patch_model());

	if (m_patch_view != NULL) {
		patch->create_module(m_patch_view->canvas());
		m_patch_view->canvas()->add_module(patch->module());
		patch->module()->resize(); 
	}
}


void
PatchController::add_node(NodeModel* nm)
{
	assert(nm != NULL);
	assert(nm->parent() == NULL);
	assert(nm->path().parent() == m_model->path());
	
	if (patch_model()->get_node(nm->name()) != NULL) {
		// Node already exists, ignore
		delete nm;
	} else {
		// FIXME: Should PatchController really be responsible for creating these?
		NodeController* nc = NULL;
		
		if (nm->plugin()->type() == PluginModel::DSSI)
			nc = new DSSIController(nm);
		else
			nc = new NodeController(nm);

		assert(nc != NULL);
		assert(nm->controller() == nc);
		
		// Check if this is a bridge node
		PortModel* const pm = patch_model()->get_port(nm->path().name());
		if (pm != NULL) {
			cerr << "Bridge node." << endl;
			PortController* pc = ((PortController*)pm->controller());
			assert(pc != NULL);
			nc->bridge_port(pc);
		}
		
		nc->add_to_store();
		patch_model()->add_node(nm);
		
		if (m_patch_view != NULL) {
			
			int x, y;
			get_new_module_location(x, y);
			nm->x(x);
			nm->y(y);

			// Set zoom to 1.0 so module isn't messed up (Death to GnomeCanvas)
			float old_zoom = m_patch_view->canvas()->zoom();
			if (old_zoom != 1.0)
				m_patch_view->canvas()->zoom(1.0);
			
			if (nc->module() == NULL)
				nc->create_module(m_patch_view->canvas());
			assert(nc->module() != NULL);
			m_patch_view->canvas()->add_module(nc->module());
			nc->module()->resize();
	
			// Reset zoom
			if (old_zoom != 1.0) {
				m_patch_view->canvas()->zoom(old_zoom);
				nc->module()->zoom(old_zoom);
			}
		}
	}
}


/** Removes a node from this patch.
 */
void
PatchController::remove_node(const string& name)
{
	assert(name.find("/") == string::npos);

	// Update breadcrumbs if necessary
	if (m_window != NULL)
		m_window->node_removed(name);
	
	if (m_patch_view != NULL) {
		assert(m_patch_view->canvas() != NULL);
		m_patch_view->canvas()->remove_module(name);
	}
	
	patch_model()->remove_node(name);
}


/** Add a port to this patch.
 *
 * Will add a port to the subpatch module and the control window, if they
 * exist.
 */
void
PatchController::add_port(PortModel* pm)
{
	assert(pm != NULL);
	assert(pm->parent() == NULL);

	//cerr << "[PatchController] Adding port " << pm->path() << endl;

	if (patch_model()->get_port(pm->name()) != NULL) {
		cerr << "[PatchController] Ignoring duplicate port "
			<< pm->path() << endl;
		delete pm;
		return;
	}
	
	node_model()->add_port(pm);
	PortController* pc = new PortController(pm);

	// Handle bridge ports/nodes (this is uglier than it should be)
	NodeController* nc = Store::instance().node(pm->path());
	if (nc != NULL)
		nc->bridge_port(pc);
	
	if (m_module != NULL) {
		pc->create_port(m_module);
		m_module->resize();
	}
		
	if (m_control_window != NULL) {
		assert(m_control_window->control_panel() != NULL);
		m_control_window->control_panel()->add_port(pc);
		m_control_window->resize();
	}

	// Enable "Controls" menuitem on module and patch window, if necessary
	if (has_control_inputs())
		enable_controls_menuitem();
}


/** Removes a port from this patch
 */
void
PatchController::remove_port(const Path& path, bool resize_module)
{
	assert(path.parent() == m_model->path());

	//cerr << "[PatchController] Removing port " << path << endl;

	/* FIXME
	if (m_control_panel != NULL) {
		m_control_panel->remove_port(path);
		if (m_control_window != NULL) {
			assert(m_control_window->control_panel() == m_control_panel);
			m_control_window->resize();
		}
	}*/
		
	// Remove port on module
	if (m_module != NULL) {
		assert(m_module->port(path.name()) != NULL);
		m_module->remove_port(path.name(), resize_module);
		assert(m_module->port(path.name()) == NULL);
	}
	
	patch_model()->remove_port(path);
	assert(patch_model()->get_port(path.name()) == NULL);
	
	// Disable "Controls" menuitem on module and patch window, if necessary
	if (!has_control_inputs())
		disable_controls_menuitem();
}


void
PatchController::connection(ConnectionModel* const cm)
{
	assert(cm != NULL);

	patch_model()->add_connection(cm);
	
	if (m_patch_view != NULL)
		create_connection(cm);
}



void
PatchController::disconnection(const Path& src_port_path, const Path& dst_port_path)
{
	const string& src_node_name = src_port_path.parent().name();
	const string& src_port_name = src_port_path.name();
	const string& dst_node_name = dst_port_path.parent().name();
	const string& dst_port_name = dst_port_path.name();
	
	if (m_patch_view != NULL)
		m_patch_view->canvas()->remove_connection(
			src_node_name, src_port_name, dst_node_name, dst_port_name);

	patch_model()->remove_connection(src_port_path, dst_port_path);
	
	// Enable control slider in destination node control window
	PortController* p = Store::instance().port(dst_port_path);
	assert(p != NULL);

	if (p->control_panel() != NULL)
		p->control_panel()->enable_port(p->path());
}


/** Try to guess a suitable location for a new module.
 */
void
PatchController::get_new_module_location(int& x, int& y)
{
	assert(m_patch_view != NULL);
	assert(m_patch_view->canvas() != NULL);
	m_patch_view->canvas()->get_scroll_offsets(x, y);
	x += 20;
	y += 20;
}


void
PatchController::show_patch_window()
{
	if (m_window == NULL) {
		Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference();
		
		xml->get_widget_derived("patch_win", m_window);
		assert(m_window != NULL);
		
		if (m_patch_view == NULL)
			create_view();

		m_window->patch_controller(this);
	}
	
	assert(m_window != NULL);
	m_window->present();
}


/** Become the parent of the patch view.
 *
 * Steals the view away from whatever window is currently showing it.
 */
void
PatchController::claim_patch_view()
{
	assert(m_patch_view != NULL);
	
	m_patch_view->hide();
	m_patch_view->reparent(m_patch_view_bin);
}


void
PatchController::show_control_window()
{
	assert(patch_model() != NULL);

	if (m_control_window == NULL)
		m_control_window = new NodeControlWindow(this, patch_model()->poly());
	
	if (m_control_window->control_panel()->num_controls() > 0)
		m_control_window->present();
}


void
PatchController::enable_controls_menuitem()
{
	if (m_window != NULL)
		m_window->menu_view_control_window()->property_sensitive() = true;
	
	NodeController::enable_controls_menuitem();
}


void
PatchController::disable_controls_menuitem()
{
	if (m_window != NULL)
		m_window->menu_view_control_window()->property_sensitive() = false;
	
	NodeController::disable_controls_menuitem();
}


} // namespace OmGtk
