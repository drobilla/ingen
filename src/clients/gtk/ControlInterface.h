/* This file is part of Om.  Copyright(C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or(at your option) any later
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

#ifndef CONTROLINTERFACE_H
#define CONTROLINTERFACE_H

#include <cassert>
#include "ModelClientInterface.h"
#include <sigc++/sigc++.h>
#include <string>
using std::string;

namespace LibOmClient
{
class PluginModel;
class PatchModel;
class NodeModel;
class PortModel;
class ConnectionModel;
}
using namespace LibOmClient;

namespace OmGtk
{

class App;


/** Provides the public interface for controlling OmGtk(via engine events).
 *
 * All control of OmGtk from the engine happens through this interface.
 *
 * All of these functions must be called in the GTK thread.  This is a unified
 * interface for controlling OmGtk(eg from the engine) but it doesn't take
 * care of any threading issues.
 *
 * \ingroup OmGtk
 */
class ControlInterface : public sigc::trackable//, public ModelClientInterface
{
public:

	ControlInterface(App* app)
	: error_slot(sigc::mem_fun(this, &ControlInterface::error))
	, new_plugin_slot(sigc::mem_fun(this, &ControlInterface::new_plugin_model))
	, new_patch_slot(sigc::mem_fun(this, &ControlInterface::new_patch_model))
	, new_node_slot(sigc::mem_fun(this, &ControlInterface::new_node_model))
	, new_port_slot(sigc::mem_fun(this, &ControlInterface::new_port_model))
	, patch_enabled_slot(sigc::mem_fun(this, &ControlInterface::patch_enabled))
	, patch_disabled_slot(sigc::mem_fun(this, &ControlInterface::patch_disabled))
	, patch_cleared_slot(sigc::mem_fun(this, &ControlInterface::patch_cleared))
	, object_destroyed_slot(sigc::mem_fun(this, &ControlInterface::object_destroyed))
	, object_renamed_slot(sigc::mem_fun(this, &ControlInterface::object_renamed))
	, connection_slot(sigc::mem_fun(this, &ControlInterface::connection_model))
	, disconnection_slot(sigc::mem_fun(this, &ControlInterface::disconnection))
	, metadata_update_slot(sigc::mem_fun(this, &ControlInterface::metadata_update))
	, control_change_slot(sigc::mem_fun(this, &ControlInterface::control_change))
	, program_add_slot(sigc::mem_fun(this, &ControlInterface::program_add))
	, program_remove_slot(sigc::mem_fun(this, &ControlInterface::program_remove))
	, _app(app)
	{
		assert(_app);
	}

	virtual ~ControlInterface() {}

	sigc::slot<void>                                            bundle_begin_slot; 
	sigc::slot<void>                                            bundle_end_slot; 
	sigc::slot<void, uint32_t>                                    num_plugins_slot; 
	sigc::slot<void, string>                                    error_slot; 
	sigc::slot<void, PluginModel*>                              new_plugin_slot; 
	sigc::slot<void, PatchModel*>                               new_patch_slot; 
	sigc::slot<void, NodeModel*>                                new_node_slot; 
	sigc::slot<void, PortModel*>                                new_port_slot; 
	sigc::slot<void, string>                                    patch_enabled_slot; 
	sigc::slot<void, string>                                    patch_disabled_slot; 
	sigc::slot<void, string>                                    patch_cleared_slot; 
	sigc::slot<void, string>                                    object_destroyed_slot; 
	sigc::slot<void, string, string>                            object_renamed_slot; 
	sigc::slot<void, ConnectionModel*>                          connection_slot; 
	sigc::slot<void, string, string>                            disconnection_slot; 
	sigc::slot<void, string, string, string>                    metadata_update_slot; 
	sigc::slot<void, string, float>                             control_change_slot; 
	sigc::slot<void, string, uint32_t, uint32_t, const string&> program_add_slot; 
	sigc::slot<void, string, uint32_t, uint32_t>                program_remove_slot; 

private:
	
	void bundle_begin(); 
	void bundle_end(); 
	void num_plugins(uint32_t); 
	void error(const string&); 
	void new_plugin_model(PluginModel*); 
	void new_patch_model(PatchModel*); 
	void new_node_model(NodeModel*); 
	void new_port_model(PortModel*); 
	void patch_enabled(const string&); 
	void patch_disabled(const string&); 
	void patch_cleared(const string&); 
	void object_destroyed(const string&); 
	void object_renamed(const string&, const string&); 
	void connection_model(ConnectionModel*); 
	void disconnection(const string&, const string&); 
	void metadata_update(const string&, const string&, const string&); 
	void control_change(const string&, float); 
	void program_add(const string&, uint32_t, uint32_t, const string&); 
	void program_remove(const string&, uint32_t, uint32_t);

	App* _app;
};


} // namespace OmGtk

#endif // CONTROLINTERFACE_H

