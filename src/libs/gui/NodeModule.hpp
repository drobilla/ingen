/* This file is part of In* Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef NODEMODULE_H
#define NODEMODULE_H

#include <string>
#include <libgnomecanvasmm.h>
#include <flowcanvas/Module.hpp>
#include <raul/SharedPtr.hpp>
#include "Port.hpp"
#include "NodeMenu.hpp"

class Atom;

namespace Ingen { namespace Client {
	class PortModel;
	class NodeModel;
	class ControlModel;
} }
using namespace Ingen::Client;

namespace Ingen {
namespace GUI {
	
class PatchCanvas;
class Port;


/** A module in a patch.
 *
 * This base class is extended for various types of modules - SubpatchModule,
 * DSSIModule, etc.
 *
 * \ingroup GUI
 */
class NodeModule : public FlowCanvas::Module
{
public:
	static boost::shared_ptr<NodeModule> create (boost::shared_ptr<PatchCanvas> canvas, SharedPtr<NodeModel> node);

	virtual ~NodeModule();

	boost::shared_ptr<Port> port(const std::string& port_name) {
		return boost::dynamic_pointer_cast<Ingen::GUI::Port>(
			Module::get_port(port_name));
	}

	virtual void store_location();

	void show_control_window();

	SharedPtr<NodeModel> node() const { return _node; }

protected:
	NodeModule(boost::shared_ptr<PatchCanvas> canvas, SharedPtr<NodeModel> node);

	virtual void on_double_click(GdkEventButton* ev) { show_control_window(); }
	virtual void on_middle_click(GdkEventButton* ev) { show_control_window(); }
	
	void rename();
	void set_metadata(const std::string& key, const Atom& value);
	
	void add_port(SharedPtr<PortModel> port, bool resize=true);
	void remove_port(SharedPtr<PortModel> port);

	void control_change(uint32_t index, float control);

	void embed_gui(bool embed);
	void gui_size_request(Gtk::Requisition* req);

	void create_menu();
	
	SharedPtr<NodeModel>   _node;
	NodeMenu*              _menu;
	SLV2UIInstance         _slv2_ui;
	Gtk::Widget*           _gui;
	Gnome::Canvas::Widget* _gui_item;
};


} // namespace GUI
} // namespace Ingen

#endif // NODEMODULE_H
