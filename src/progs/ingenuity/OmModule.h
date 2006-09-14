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

#ifndef OMMODULE_H
#define OMMODULE_H

#include <string>
#include <libgnomecanvasmm.h>
#include <flowcanvas/Module.h>
#include "NodeMenu.h"
#include "util/CountedPtr.h"
using std::string;

class Atom;

namespace Ingen { namespace Client {
	class PortModel;
	class NodeModel;
	class ControlModel;
} }
using namespace Ingen::Client;

namespace Ingenuity {
	
class OmFlowCanvas;
class OmPort;


/** A module in a patch.
 *
 * This base class is extended for various types of modules - SubpatchModule,
 * DSSIModule, etc.
 *
 * \ingroup Ingenuity
 */
class OmModule : public LibFlowCanvas::Module
{
public:
	OmModule(OmFlowCanvas* canvas, CountedPtr<NodeModel> node);
	virtual ~OmModule() {}
	
	virtual OmPort* port(const string& port_name) {
		return (OmPort*)Module::get_port(port_name);
	}

	virtual void store_location();

	void on_right_click(GdkEventButton* event);
	
	void show_control_window();

	CountedPtr<NodeModel> node() const { return m_node; }

protected:
	virtual void on_double_click(GdkEventButton* ev) { show_control_window(); }
	virtual void on_middle_click(GdkEventButton* ev) { show_control_window(); }
	
	void metadata_update(const string& key, const Atom& value);

	void create_all_ports();
	void add_port(CountedPtr<PortModel> port);
	void remove_port(CountedPtr<PortModel> port);

	CountedPtr<NodeModel> m_node;
	NodeMenu              m_menu;
};


} // namespace Ingenuity

#endif // OMMODULE_H
