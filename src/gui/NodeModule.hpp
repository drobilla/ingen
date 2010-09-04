/* This file is part of In* Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_GUI_NODEMODULE_HPP
#define INGEN_GUI_NODEMODULE_HPP

#include <string>
#include <libgnomecanvasmm.h>
#include "flowcanvas/Module.hpp"
#include "raul/SharedPtr.hpp"
#include "Port.hpp"

namespace Raul { class Atom; }

namespace Ingen { namespace Client {
	class NodeModel;
	class PluginUI;
	class PortModel;
} }
using namespace Ingen::Client;

namespace Ingen {
namespace GUI {

class PatchCanvas;
class Port;
class NodeMenu;


/** A module in a patch.
 *
 * This base class is extended for various types of modules.
 *
 * \ingroup GUI
 */
class NodeModule : public FlowCanvas::Module
{
public:
	static boost::shared_ptr<NodeModule> create (
			boost::shared_ptr<PatchCanvas> canvas,
			SharedPtr<NodeModel> node,
			bool human_names);

	virtual ~NodeModule();

	boost::shared_ptr<Port> port(boost::shared_ptr<PortModel> model);

	void remove_port(SharedPtr<PortModel> port);

	virtual void store_location();
	void show_human_names(bool b);

	SharedPtr<NodeModel> node() const { return _node; }

protected:
	NodeModule(boost::shared_ptr<PatchCanvas> canvas, SharedPtr<NodeModel> node);

	void on_double_click(GdkEventButton* ev);

	void show_control_window();
	void embed_gui(bool embed);
	bool popup_gui();
	void on_gui_window_close();
	void set_selected(bool b);

	void rename();
	void property_changed(const Raul::URI& predicate, const Raul::Atom& value);

	void add_port(SharedPtr<PortModel> port, bool resize=true);

	void value_changed(uint32_t index, const Raul::Atom& value);
	void plugin_changed();
	void set_control_values();

	void create_menu();

	SharedPtr<NodeModel>   _node;
	NodeMenu*              _menu;
	SharedPtr<PluginUI>    _plugin_ui;
	Gtk::Widget*           _gui_widget;
	Gtk::Window*           _gui_window; ///< iff popped up
};


} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_NODEMODULE_HPP
