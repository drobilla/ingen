/*
  This file is part of Ingen.
  Copyright 2014 David Robillard <http://drobilla.net/>

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

#include <map>
#include <set>
#include <string>

#include <gtkmm/menu.h>

#include "ingen/World.hpp"
#include "ingen/types.hpp"
#include "lilv/lilv.h"

namespace Ingen {

namespace Client { class PluginModel; }

namespace GUI {

/**
   Type-hierarchical plugin menu.

   @ingroup GUI
*/
class PluginMenu : public Gtk::Menu
{
public:
	PluginMenu(Ingen::World& world);

	void add_plugin(SPtr<Client::PluginModel> p);

	sigc::signal< void, WPtr<Client::PluginModel> > signal_load_plugin;

private:
	struct MenuRecord {
		MenuRecord(Gtk::MenuItem* i, Gtk::Menu* m) : item(i), menu(m) {}
		Gtk::MenuItem* item;
		Gtk::Menu*     menu;
	};

	typedef std::multimap<const std::string, const LilvPluginClass*> LV2Children;
	typedef std::multimap<const std::string, MenuRecord>             ClassMenus;

	/// Recursively add hierarchy rooted at `plugin_class` to `menu`.
	size_t build_plugin_class_menu(Gtk::Menu*               menu,
	                               const LilvPluginClass*   plugin_class,
	                               const LilvPluginClasses* classes,
	                               const LV2Children&       children,
	                               std::set<const char*>&   ancestors);

	void add_plugin_to_menu(MenuRecord& menu, SPtr<Client::PluginModel> p);

	void load_plugin(WPtr<Client::PluginModel> weak_plugin);

	Ingen::World& _world;
	MenuRecord    _classless_menu;
	ClassMenus    _class_menus;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_PLUGINMENU_HPP
