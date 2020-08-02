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

#ifndef INGEN_GUI_NODEMENU_HPP
#define INGEN_GUI_NODEMENU_HPP

#include "ObjectMenu.hpp"

#include "ingen/client/BlockModel.hpp"
#include "ingen/memory.hpp"

#include <gtkmm/builder.h>
#include <gtkmm/menu.h>
#include <gtkmm/menushell.h>

#include <memory>
#include <string>

namespace ingen {
namespace gui {

/** Menu for a Node.
 *
 * \ingroup GUI
 */
class NodeMenu : public ObjectMenu
{
public:
	NodeMenu(BaseObjectType*                   cobject,
	         const Glib::RefPtr<Gtk::Builder>& xml);

	void init(App& app, SPtr<const client::BlockModel> block);

	bool has_control_inputs();

	sigc::signal<void>       signal_popup_gui;
	sigc::signal<void, bool> signal_embed_gui;

protected:
	SPtr<const client::BlockModel> block() const {
		return std::dynamic_pointer_cast<const client::BlockModel>(_object);
	}

	void add_preset(const URI& uri, const std::string& label);

	void on_menu_disconnect() override;
	void on_menu_embed_gui();
	void on_menu_enabled();
	void on_menu_randomize();
	void on_save_preset_activated();
	void on_preset_activated(const std::string& uri);

	Gtk::MenuItem*      _popup_gui_menuitem;
	Gtk::CheckMenuItem* _embed_gui_menuitem;
	Gtk::CheckMenuItem* _enabled_menuitem;
	Gtk::MenuItem*      _randomize_menuitem;
	Gtk::Menu*          _presets_menu;
	sigc::connection    _preset_connection;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_NODEMENU_HPP
