/*
  This file is part of Ingen.
  Copyright 2014-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_PLUGINMENU_HPP
#define INGEN_GUI_PLUGINMENU_HPP

#include <lilv/lilv.h>

#include <gtkmm/menu.h>
#include <sigc++/signal.h>

#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <string>

namespace Gtk {
class MenuItem;
} // namespace Gtk

namespace ingen {

class World;

namespace client {
class PluginModel;
} // namespace client

namespace gui {

/**
   Type-hierarchical plugin menu.

   @ingroup GUI
*/
class PluginMenu : public Gtk::Menu
{
public:
	explicit PluginMenu(ingen::World& world);

	void clear();
	void add_plugin(const std::shared_ptr<client::PluginModel>& p);

	sigc::signal<void, std::weak_ptr<client::PluginModel>> signal_load_plugin;

private:
	struct MenuRecord {
		MenuRecord(Gtk::MenuItem* i, Gtk::Menu* m) noexcept : item(i), menu(m)
		{}

		Gtk::MenuItem* item;
		Gtk::Menu*     menu;
	};

	using LV2Children = std::multimap<const std::string, const LilvPluginClass*>;
	using ClassMenus  = std::multimap<const std::string, MenuRecord>;

	/// Recursively add hierarchy rooted at `plugin_class` to `menu`.
	size_t build_plugin_class_menu(Gtk::Menu*               menu,
	                               const LilvPluginClass*   plugin_class,
	                               const LilvPluginClasses* classes,
	                               const LV2Children&       children,
	                               std::set<const char*>&   ancestors);

	void add_plugin_to_menu(MenuRecord&                                 menu,
	                        const std::shared_ptr<client::PluginModel>& p);

	void load_plugin(const std::weak_ptr<client::PluginModel>& weak_plugin);

	ingen::World& _world;
	MenuRecord    _classless_menu;
	ClassMenus    _class_menus;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_PLUGINMENU_HPP
