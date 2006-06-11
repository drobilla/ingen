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

#ifndef PORTCONTROLLER_H
#define PORTCONTROLLER_H

#include <string>
#include <gtkmm.h>
#include "GtkObjectController.h"

using std::string;
using namespace LibOmClient;

namespace LibOmClient {
	class MetadataModel;
	class PortModel;
}

namespace OmGtk {

class Controller;
class OmPort;
class ControlPanel;
class OmModule;


/** Controller for a port on a (non-patch) node.
 *
 * \ingroup OmGtk
 */
class PortController : public GtkObjectController
{
public:
	PortController(PortModel* model);
	virtual ~PortController() {}
	
	virtual void destroy();

/*
	virtual void add_to_store();
	virtual void remove_from_store();
*/
	virtual void metadata_update(const string& key, const string& value);
	
	void create_port(OmModule* module);
	void set_path(const Path& new_path);
	
	void control_change(float value);
	
	ControlPanel* control_panel() const { return m_control_panel; }
	void set_control_panel(ControlPanel* cp);

	CountedPtr<PortModel> port_model() const { return CountedPtr<PortModel>((PortModel*)m_model.get()); }

private:
	OmPort*       m_port;          ///< Canvas module port
	ControlPanel* m_control_panel; ///< Control panel that contains this port
};


} // namespace OmGtk

#endif // PORTCONTROLLER_H
