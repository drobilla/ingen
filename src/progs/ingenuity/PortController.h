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

#ifndef PORTCONTROLLER_H
#define PORTCONTROLLER_H

#include <string>
#include <gtkmm.h>
#include "GtkObjectController.h"
#include "OmPortModule.h"

using std::string;
using namespace Ingen::Client;

namespace Ingen { namespace Client {
	class MetadataModel;
	class PortModel;
} }

namespace Ingenuity {

class Controller;
class OmPort;
class OmPatchPort;
//class ControlPanel;
class OmModule;
class OmPortModule;
class OmFlowCanvas;


/** Controller for a port on a (non-patch) node.
 *
 * \ingroup Ingenuity
 */
class PortController : public GtkObjectController
{
public:
	PortController(CountedPtr<PortModel> model);
	virtual ~PortController() {}
	
	virtual void destroy();

	virtual void create_module(OmFlowCanvas* canvas);
	OmPortModule* module() { return m_module; }
/*
	virtual void add_to_store();
	virtual void remove_from_store();
*/
	virtual void metadata_update(const string& key, const string& value);
	
	void create_port(OmModule* module);
	void set_path(const Path& new_path);
	
	//ControlPanel* control_panel() const { return m_control_panel; }
	//void set_control_panel(ControlPanel* cp);

	CountedPtr<PortModel> port_model() const { return m_model; }

private:
	OmPatchPort*  m_patch_port;    ///< Port on m_module
	OmPortModule* m_module;        ///< Port pseudo-module (for patch ports only)
	OmPort*       m_port;          ///< Port on some other canvas module
	//ControlPanel* m_control_panel; ///< Control panel that contains this port
};


} // namespace Ingenuity

#endif // PORTCONTROLLER_H
