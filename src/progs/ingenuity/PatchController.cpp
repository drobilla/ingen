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
#include "SubpatchModule.h"
#include "DSSIModule.h"
#include "PatchWindow.h"
#include "NodeModel.h"
#include "OmModule.h"
#include "OmPortModule.h"
#include "OmPort.h"
#include "ControlModel.h"
#include "NodeControlWindow.h"
#include "NodeController.h"
#include "PortController.h"
#include "App.h"
#include "PatchTreeWindow.h"
#include "PatchPropertiesWindow.h"
#include "DSSIController.h"
#include "PatchModel.h"
#include "Store.h"

using std::cerr; using std::cout; using std::endl;
using namespace Ingen::Client;

namespace Ingenuity {


PatchController::PatchController(CountedPtr<PatchModel> model)
: NodeController(model),
  m_properties_window(NULL),
  m_window(NULL),
  m_patch_view(NULL),
  m_patch_model(model),
  m_module_x(0),
  m_module_y(0)
{
	assert(model->path().length() > 0);
	assert(model->controller() == this); // NodeController() does this
	assert(m_patch_model == model);

/* FIXME	if (model->path() != "/") {
		PatchController* parent = Store::instance().patch(model->path().parent());
		if (parent != NULL)
			parent->add_subpatch(this);
		else
			cerr << "[PatchController] " << path() << " ERROR: Parent not found." << endl;
	}*/

	//model->new_port_sig.connect(sigc::mem_fun(this, &PatchController::add_port));
	model->new_node_sig.connect(sigc::mem_fun(this, &PatchController::add_node));
	model->removed_node_sig.connect(sigc::mem_fun(this, &PatchController::remove_node));
	model->new_connection_sig.connect(sigc::mem_fun(this, &PatchController::connection));
	model->removed_connection_sig.connect(sigc::mem_fun(this, &PatchController::disconnection));
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
	//Store::instance().remove_object(this);
	
	// Delete self from parent (this will delete model)
	/*if (patch_model()->parent() != NULL) {
		PatchController* const parent = (PatchController*)patch_model()->parent()->controller();
		assert(parent != NULL);
		parent->remove_node(name());
	} else {
		//delete m_model;
	}*/
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
	assert(m_model);
	Path old_path = path();
	
	// Rename nodes
	for (NodeModelMap::const_iterator i = patch_model()->nodes().begin();
			i != patch_model()->nodes().end(); ++i) {
		const NodeModel* const nm = (*i).second.get();
		assert(nm );
		NodeController* const nc = ((NodeController*)nm->controller());
		assert(nc );
		nc->set_path(new_path.base_path() + nc->node_model()->name());
	}
	
#ifdef DEBUG
	// Be sure ports were renamed by their bridge nodes
	for (PortModelList::const_iterator i = node_model()->ports().begin();
			i != node_model()->ports().end(); ++i) {
		GtkObjectController* const pc = (GtkObjectController*)((*i)->controller());
		assert(pc );
		assert(pc->path().parent()== new_path);
	}
#endif
	
	App::instance().patch_tree()->patch_renamed(old_path, new_path);
	
	if (m_window)
		m_window->patch_renamed(new_path);

	if (m_control_window)
		m_control_window->set_title(new_path + " Controls");

	if (m_module)
		m_module->name(new_path.name());
	
	PatchController* parent = dynamic_cast<PatchController*>(
		patch_model()->parent()->controller());
	
	if (parent && parent->window())
		parent->window()->node_renamed(old_path, new_path);

	//remove_from_store();
	GtkObjectController::set_path(new_path);
	//add_to_store();
	
	if (old_path.name() != new_path.name())
		parent->patch_model()->rename_node(old_path, new_path);
}


void
PatchController::create_module(OmFlowCanvas* canvas)
{
	// Update menu if we didn't used to have a module
	if (!m_module) {
		/*Gtk::Menu::MenuList& items = m_menu.items();
		m_menu.remove(items[4]);

		items.push_front(Gtk::Menu_Helpers::SeparatorElem());
		items.push_front(Gtk::Menu_Helpers::MenuElem("Browse to Patch",
					sigc::mem_fun((SubpatchModule*)m_module, &SubpatchModule::browse_to_patch)));
		items.push_front(Gtk::Menu_Helpers::MenuElem("Open Patch in New Window",
					sigc::mem_fun(this, &PatchController::show_patch_window)));*/
	}

	if (!m_module || m_module->canvas() != canvas) {

		//cerr << "Creating patch module " << m_model->path() << endl;

		assert(canvas != NULL);
		assert(m_module == NULL);
		assert(!m_patch_view || canvas != m_patch_view->canvas());

		m_module = new SubpatchModule(canvas, this);
		create_all_ports();
	}

	m_module->move_to(node_model()->x(), node_model()->y());
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

		NodeModel* const nm = (*i).second.get();

		string val = nm->get_metadata("module-x");
		if (val != "")
			nm->x(atof(val.c_str()));
		val = nm->get_metadata("module-y");
		if (val != "")
			nm->y(atof(val.c_str()));

		/* Set sane default coordinates if not set already yet */
		if (nm->x() == 0.0f && nm->y() == 0.0f) {
			double x, y;
			m_patch_view->canvas()->get_new_module_location(x, y);
			nm->x(x);
			nm->y(y);
		}

		NodeController* nc = ((NodeController*)nm->controller());
		if (!nc)
			nc = create_controller_for_node(nm);

		assert(nc);
		assert(nm->controller() == nc);
		
		nc->create_module(m_patch_view->canvas());
		assert(nc->module());
	}

	// Create pseudo modules for ports (ports on this canvas, not on our module)
	for (PortModelList::const_iterator i = patch_model()->ports().begin();
			i != patch_model()->ports().end(); ++i) {
		PortController* const pc = dynamic_cast<PortController*>((*i)->controller());
		assert(pc);
		pc->create_module(m_patch_view->canvas());
	}


	// Create connections
	for (list<CountedPtr<ConnectionModel> >::const_iterator i = patch_model()->connections().begin();
			i != patch_model()->connections().end(); ++i) {
		connection(*i);
	}

	// Set run checkbox
	if (patch_model()->enabled())
		m_patch_view->enable();
}


void
PatchController::show_properties_window()
{
	if (!m_properties_window) {
		Glib::RefPtr<Gnome::Glade::Xml> glade_xml = GladeFactory::new_glade_reference();
		glade_xml->get_widget_derived("patch_properties_win", m_properties_window);
		m_properties_window->patch_model(patch_model());
	}
	
	m_properties_window->show();

}


/** Create a connection in the view (canvas).
 */
void
PatchController::connection(CountedPtr<ConnectionModel> cm)
{
	if (m_patch_view != NULL) {

		// Deal with port "anonymous nodes" for this patch's own ports...
		const Path& src_parent_path = cm->src_port_path().parent();
		const Path& dst_parent_path = cm->dst_port_path().parent();

		const string& src_parent_name =
			(src_parent_path == path()) ? "" : src_parent_path.name();
		const string& dst_parent_name =
			(dst_parent_path == path()) ? "" : dst_parent_path.name();

		Port* src_port = m_patch_view->canvas()->get_port(src_parent_name, cm->src_port_path().name());
		Port* dst_port = m_patch_view->canvas()->get_port(dst_parent_name, cm->dst_port_path().name());
		assert(src_port && dst_port);

		m_patch_view->canvas()->add_connection(src_port, dst_port);
	}
}


NodeController*
PatchController::create_controller_for_node(CountedPtr<NodeModel> node)
{
	assert(!node->controller());
	NodeController* nc = NULL;

	CountedPtr<PatchModel> patch(node);
	if (patch) {
		assert(patch == node);
		assert(patch->parent() == m_patch_model);
		nc = new PatchController(patch);
	} else {
		assert(node->plugin());
		if (node->plugin()->type() == PluginModel::DSSI)
			nc = new DSSIController(node);
		else
			nc = new NodeController(node);
	}

	assert(node->controller() == nc);
	return nc;
}


/** Add a child node to this patch.
 *
 * This is for plugin nodes and patches, and is responsible for creating the 
 * GtkObjectController for @a object (and through that the View if necessary)
 */
void
PatchController::add_node(CountedPtr<NodeModel> object)
{
	assert(object);
	assert(object->parent() == m_patch_model);
	assert(patch_model()->get_node(object->name()));
	
	/*if (patch_model()->get_node(nm->name()) != NULL) {
		cerr << "Ignoring existing\n";
		// Node already exists, ignore
		//delete nm;
		} else {*/


	CountedPtr<NodeModel> node(object);
	if (node) {
		assert(node->parent() == m_patch_model);

		NodeController* nc = create_controller_for_node(node);
		assert(nc);
		assert(node->controller() == nc);

		if (m_patch_view != NULL) {
			double x, y;
			m_patch_view->canvas()->get_new_module_location(x, y);
			node->x(x);
			node->y(y);

			// Set zoom to 1.0 so module isn't messed up (Death to GnomeCanvas)
			float old_zoom = m_patch_view->canvas()->zoom();
			if (old_zoom != 1.0)
				m_patch_view->canvas()->zoom(1.0);

			nc->create_module(m_patch_view->canvas());
			assert(nc->module());
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
	assert(!m_patch_model->get_node(name));

	// Update breadcrumbs if necessary
	if (m_window)
		m_window->node_removed(name);
}


/** Add a port to this patch.
 *
 * Will add a port to the subpatch module and the control window, if they
 * exist.
 */
void
PatchController::add_port(CountedPtr<PortModel> pm)
{
	assert(pm);
	assert(pm->parent() == m_patch_model);
	assert(patch_model()->get_port(pm->name()));
	
	//cerr << "[PatchController] Adding port " << pm->path() << endl;

	/*if (patch_model()->get_port(pm->name())) {
	  cerr << "[PatchController] Ignoring duplicate port "
	  << pm->path() << endl;
	  return;
	  }*/

	//node_model()->add_port(pm);
	// FIXME: leak
	PortController* pc = new PortController(pm);

	// Handle bridge ports/nodes (this is uglier than it should be)
	/*NodeController* nc = (NodeController*)Store::instance().node(pm->path())->controller();
	  if (nc != NULL)
	  nc->bridge_port(pc);
	  */

	// Create port on this patch's module (if there is one)
	if (m_module != NULL) {
		pc->create_port(m_module);
		m_module->resize();
	}	

	// Create port's (pseudo) module on this patch's canvas (if there is one)
	if (m_patch_view != NULL) {

		// Set zoom to 1.0 so module isn't messed up (Death to GnomeCanvas)
		float old_zoom = m_patch_view->canvas()->zoom();
		if (old_zoom != 1.0)
			m_patch_view->canvas()->zoom(1.0);

		pc->create_module(m_patch_view->canvas());

		// Reset zoom
		if (old_zoom != 1.0) {
			m_patch_view->canvas()->zoom(old_zoom);
			pc->module()->zoom(old_zoom);
		}
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
	assert( ! patch_model()->get_port(path.name()));

	//cerr << "[PatchController] Removing port " << path << endl;

	/* FIXME
	if (m_control_panel != NULL) {
		m_control_panel->remove_port(path);
		if (m_control_window != NULL) {
			assert(m_control_window->control_panel() == m_control_panel);
			m_control_window->resize();
		}
	}*/
		
	if (m_module) {
		delete m_module->port(path.name());
		if (resize_module)
			m_module->resize();
	}
	
	// Disable "Controls" menuitem on module and patch window, if necessary
	if (!has_control_inputs())
		disable_controls_menuitem();
}


void
PatchController::disconnection(const Path& src_port_path, const Path& dst_port_path)
{
	const string& src_node_name = src_port_path.parent().name();
	const string& src_port_name = src_port_path.name();
	const string& dst_node_name = dst_port_path.parent().name();
	const string& dst_port_name = dst_port_path.name();
	
	if (m_patch_view) {
		Port* src_port = m_patch_view->canvas()->get_port(src_node_name, src_port_name);
		Port* dst_port = m_patch_view->canvas()->get_port(dst_node_name, dst_port_name);
		
		if (src_port && dst_port) {
			m_patch_view->canvas()->remove_connection(src_port, dst_port);
		}
	}

	//patch_model()->remove_connection(src_port_path, dst_port_path);
	
	cerr << "FIXME: disconnection\n";
	/*
	// Enable control slider in destination node control window
	PortController* p = (PortController)Store::instance().port(dst_port_path)->controller();
	assert(p != NULL);

	if (p->control_panel() != NULL)
		p->control_panel()->enable_port(p->path());
		*/
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
	assert(patch_model());

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


} // namespace Ingenuity
