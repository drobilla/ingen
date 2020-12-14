/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

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

#include "Port.hpp"

#include "ganv/Module.hpp"

#include <memory>

namespace ingen {
namespace client {
class BlockModel;
class PluginUI;
class PortModel;
} // namespace client
} // namespace ingen

namespace ingen {
namespace gui {

class GraphCanvas;
class Port;
class NodeMenu;

/** A module in a graphn.
 *
 * This base class is extended for various types of modules.
 *
 * \ingroup GUI
 */
class NodeModule : public Ganv::Module
{
public:
	static NodeModule*
	create(GraphCanvas&                                     canvas,
	       const std::shared_ptr<const client::BlockModel>& block,
	       bool                                             human);

	~NodeModule() override;

	App& app() const;

	Port* port(const std::shared_ptr<const client::PortModel>& model);

	void
	delete_port_view(const std::shared_ptr<const client::PortModel>& model);

	virtual void store_location(double ax, double ay);
	void show_human_names(bool b);

	std::shared_ptr<const client::BlockModel> block() const { return _block; }

protected:
	NodeModule(GraphCanvas&                                     canvas,
	           const std::shared_ptr<const client::BlockModel>& block);

	virtual bool on_double_click(GdkEventButton* ev);

	bool idle_init();
	bool on_event(GdkEvent* ev) override;

	void on_embed_gui_toggled(bool embed);
	void embed_gui(bool embed);
	bool popup_gui();
	void on_gui_window_close();
	bool on_selected(gboolean selected) override;

	void rename();
	void property_changed(const URI& key, const Atom& value);

	void new_port_view(const std::shared_ptr<const client::PortModel>& port);

	void port_activity(uint32_t index, const Atom& value);
	void port_value_changed(uint32_t index, const Atom& value);
	void plugin_changed();
	void set_control_values();

	bool show_menu(GdkEventButton* ev);

	std::shared_ptr<const client::BlockModel> _block;
	NodeMenu*                                 _menu;
	std::shared_ptr<client::PluginUI>         _plugin_ui;
	Gtk::Widget*                              _gui_widget;
	Gtk::Window*                              _gui_window; ///< iff popped up
	bool                                      _initialised;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_NODEMODULE_HPP
