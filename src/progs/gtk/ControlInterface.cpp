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

#include <cassert>

#include "ControlInterface.h"
#include "App.h"

#include "PluginModel.h"
#include "PatchModel.h"
#include "NodeModel.h"
#include "PortModel.h"
#include "ConnectionModel.h"

#include "PatchController.h"
#include "PortController.h"
#include "PatchTreeWindow.h"
#include "Store.h"

namespace OmGtk
{


void
ControlInterface::error(const string& msg)
{
	_app->error_message(msg);
}


void
ControlInterface::new_plugin_model(PluginModel* pm)
{
	cerr << "NEW PLUGIN\n";
	//Store::instance().add_plugin(pm);
}


void
ControlInterface::new_patch_model(PatchModel* const pm)
{
	assert(pm);

	//cout << "[ControlInterface] New patch." << endl;

	if (Store::instance().patch(pm->path())) {
		delete pm;
	} else {

		// See if we cached this patch model to store its location (to avoid the
		// module "jumping") and filename (which isn't sent to engine)
		/*PatchModel* pm = _controller->yank_added_patch(pm->path());
		if (pm != NULL) {
			assert(pm->path() == pm->path());
			// FIXME: ew
			if (pm->parent() != NULL)
				pm->set_parent(NULL);
			const PluginModel* plugin = pm->plugin();
			*pm = *pm;
			pm->plugin(plugin);
		}*/

		assert(!pm->parent());
		PatchController* patch = new PatchController(pm);
		//Store::instance().add_object(patch);
		//_app->patch_tree()->add_patch(patch);

		if (pm->path() == "/")
			patch->show_patch_window();

		//_app->add_patch(pm, show);
	}
}


void
ControlInterface::new_node_model(NodeModel* const nm)
{
	assert(nm);

	cerr << "[ControlInterface] FIXME: New node: " << nm->name() << endl;
#if 0
	PatchController* const pc = Store::instance().patch(nm->path().parent());

	if (pc != NULL) {
		/* FIXME: this is crazy slow, and slows down patch loading too much.  Define some
		 * kind of unified string to describe all plugins (ie ladspa:amp.so:amp_gaia) and
		 * just store that in node models, and look it up when needed */

		/* Bit of a hack, throw away the placeholder PluginModel in the NodeModel
		 * and set it to the nice complete one we have stored */

		for (map<string, const PluginModel*>::const_iterator i = Store::instance().plugins().begin();
		        i != Store::instance().plugins().end(); ++i) {
			if ((*i).second->uri() == nm->plugin()->uri()) {
				// FIXME: EVIL
				delete (PluginModel*)(nm->plugin());
				nm->plugin((*i).second);
				break;
			}
		}

		pc->add_node(nm);

	} else {
		cerr << "[NewNode] Can not find parent of " << nm->path()
		<< ".  Module will not appear." << endl;
	}
	#endif
}


void
ControlInterface::new_port_model(PortModel* const pm)
{
	assert(pm);

	cout << "[ControlInterface] FIXME: New port." << endl;
/*
	NodeController* node = Store::instance().node(pm->path().parent());
	if (node != NULL)
		node->add_port(pm);
	else
		cerr << "[NewPort] Could not find parent for "
		<< pm->path() << endl;
		*/
}


void
ControlInterface::patch_enabled(const string& path)
{
	//cout << "[ControlInterface] Patch enabled." << endl;
/*
	PatchController* patch = Store::instance().patch(path);
	if (patch != NULL)
		patch->enable();
	else
		cerr << "[PatchEnabled] Cannot find patch " << path << endl;
		*/
}


void
ControlInterface::patch_disabled(const string& path)
{
	//cout << "[ControlInterface] Patch disabled." << endl;
/*
	PatchController* patch = Store::instance().patch(path);
	if (patch != NULL)
		patch->disable();
	else
		cerr << "[PatchDisabled] Cannot find patch " << path << endl;
		*/
}


void
ControlInterface::patch_cleared(const string& path)
{
	//cout << "[ControlInterface] Patch cleared." << endl;
/*
	PatchController* patch = Store::instance().patch(path);
	if (patch != NULL)
		patch->clear();
	else
		cerr << "[PatchCleared] Cannot find patch " << path << endl;
		*/
}


void
ControlInterface::object_destroyed(const string& path)
{
	//cout << "[ControlInterface] Destroying." << endl;
/*
	GtkObjectController* object = Store::instance().object(path);
	if (object != NULL) {
		object->destroy();
		delete object;
	} else {
		cerr << "[Destroy] Cannot find object " << path << endl;
	}
	*/
}


void
ControlInterface::object_renamed(const string& old_path, const string& new_path)
{
	//cout << "[ControlInterface] Renaming." << endl;
/*
	GtkObjectController* object = Store::instance().object(old_path);
	if (object != NULL)
		object->set_path(new_path);
	else
		cerr << "[ObjectRenamed] Can not find object " << old_path
		<< " to rename." << endl;
		*/
}


void
ControlInterface::connection_model(ConnectionModel* connection)
{
	assert(connection);

	//cout << "[ControlInterface] Connection" << endl;
/*
	assert(connection->src_port_path().parent().parent()
	       == connection->dst_port_path().parent().parent());

	PatchController* pc = Store::instance().patch(connection->patch_path());

	if (pc != NULL)
		pc->connection(connection);
	else
		cerr << "[Connection] Can not find patch " << connection->patch_path()
		<< ".  Connection will not be made." << endl;
		*/
}


void
ControlInterface::disconnection(const string& src_port_path, const string& dst_port_path)
{
	//cerr << "[ControlInterface] Disconnection." << endl;
/*	string patch_path = src_port_path;
	patch_path = patch_path.substr(0, patch_path.find_last_of("/"));
	patch_path = patch_path.substr(0, patch_path.find_last_of("/"));

	if (patch_path == "")
		patch_path = "/";

	PatchController* pc = Store::instance().patch(patch_path);

	if (pc != NULL)
		pc->disconnection(src_port_path, dst_port_path);
	else
		cerr << "[Disconnection] Can not find window for patch " << patch_path
		<< ".  Connection will not be removed." << endl;
		*/
}


void
ControlInterface::metadata_update(const string& path, const string& key, const string& value)
{ /*
	//cerr << "[ControlInterface] Metadata." << endl;
	GtkObjectController* object = Store::instance().object(path);
	if (object != NULL)
		object->metadata_update(key, value);
	else
		cerr << "[MetadataUpdate] Could not find object " << path << endl;
*/ }


void
ControlInterface::control_change(const string& port_path, float value)
{ /*
	//cerr << "[ControlInterface] Control change." << endl;

	PortController* port = Store::instance().port(port_path);
	if (port != NULL)
		port->control_change(value);
	else
		cerr << "[ControlChange] Can not find port " << port_path << endl;
*/ }


void
ControlInterface::program_add(const string& path, uint32_t bank, uint32_t program, const string& name)
{ /*
	NodeController* node = Store::instance().node(path);
	if (node != NULL) {
		node->program_add(bank, program, name);
		return;
	} else {
		cerr << "[ProgramAdd] Can not find node " << path << endl;
	}
*/ }


void
ControlInterface::program_remove(const string& path, uint32_t bank, uint32_t program)
{ /*
	NodeController* node = Store::instance().node(path);
	if (node != NULL) {
		node->program_remove(bank, program);
		return;
	} else {
		cerr << "[ProgramRemove] Can not find node " << path << endl;
	}
*/ }


} // namespace OmGtk
