/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_NODEMODULE_HPP
#define INGEN_GUI_NODEMODULE_HPP

#include "ganv/Module.hpp"
#include "raul/SharedPtr.hpp"

#include "Port.hpp"

namespace Raul { class Atom; }

namespace Ingen { namespace Client {
	class NodeModel;
	class PluginUI;
	class PortModel;
} }

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
class NodeModule : public Ganv::Module
{
public:
	static NodeModule* create(
		PatchCanvas&                       canvas,
		SharedPtr<const Client::NodeModel> node,
		bool                               human_names);

	virtual ~NodeModule();

	App& app() const;

	Port* port(boost::shared_ptr<const Client::PortModel> model);

	void delete_port_view(SharedPtr<const Client::PortModel> port);

	virtual void store_location(double x, double y);
	void show_human_names(bool b);
	void set_selected(gboolean b);

	SharedPtr<const Client::NodeModel> node() const { return _node; }

protected:
	NodeModule(PatchCanvas& canvas, SharedPtr<const Client::NodeModel> node);

	virtual bool on_double_click(GdkEventButton* ev);

	bool on_event(GdkEvent* ev);

	void on_embed_gui_toggled(bool embed);
	void embed_gui(bool embed);
	bool popup_gui();
	void on_gui_window_close();

	void rename();
	void property_changed(const Raul::URI& predicate, const Raul::Atom& value);

	void new_port_view(SharedPtr<const Client::PortModel> port);

	void value_changed(uint32_t index, const Raul::Atom& value);
	void port_activity(uint32_t index, const Raul::Atom& value);
	void plugin_changed();
	void set_control_values();

	bool show_menu(GdkEventButton* ev);

	SharedPtr<const Client::NodeModel> _node;
	NodeMenu*                          _menu;
	SharedPtr<Client::PluginUI>        _plugin_ui;
	Gtk::Widget*                       _gui_widget;
	Gtk::Window*                       _gui_window; ///< iff popped up
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_NODEMODULE_HPP
