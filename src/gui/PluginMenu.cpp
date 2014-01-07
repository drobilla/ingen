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

#include "PluginMenu.hpp"
#include "ingen/Log.hpp"
#include "ingen/client/PluginModel.hpp"

namespace Ingen {
namespace GUI {

PluginMenu::PluginMenu(Ingen::World& world)
	: _world(world)
	, _classless_menu(NULL, NULL)
{
	const LilvWorld*         lworld     = _world.lilv_world();
	const LilvPluginClass*   lv2_plugin = lilv_world_get_plugin_class(lworld);
	const LilvPluginClasses* classes    = lilv_world_get_plugin_classes(lworld);

	LV2Children children;
	LILV_FOREACH(plugin_classes, i, classes) {
		const LilvPluginClass* c = lilv_plugin_classes_get(classes, i);
		const LilvNode*        p = lilv_plugin_class_get_parent_uri(c);
		if (!p) {
			p = lilv_plugin_class_get_uri(lv2_plugin);
		}
		children.insert(std::make_pair(lilv_node_as_string(p), c));
	}

	std::set<const char*> ancestors;
	build_plugin_class_menu(this, lv2_plugin, classes, children, ancestors);

	items().push_back(Gtk::Menu_Helpers::MenuElem("_Uncategorized"));
	_classless_menu.item = &(items().back());
	_classless_menu.menu = new Gtk::Menu();
	_classless_menu.item->set_submenu(*_classless_menu.menu);
	_classless_menu.item->hide();
}

void
PluginMenu::add_plugin(SPtr<Client::PluginModel> p)
{
	typedef ClassMenus::iterator iterator;

	if (!p->lilv_plugin() || lilv_plugin_is_replaced(p->lilv_plugin())) {
		return;
	}

	const LilvPluginClass* pc            = lilv_plugin_get_class(p->lilv_plugin());
	const LilvNode*        class_uri     = lilv_plugin_class_get_uri(pc);
	const char*            class_uri_str = lilv_node_as_string(class_uri);

	std::pair<iterator, iterator> range = _class_menus.equal_range(class_uri_str);
	if (range.first == _class_menus.end() || range.first == range.second
	    || range.first->second.menu == this) {
		// Add to uncategorized plugin menu
		add_plugin_to_menu(_classless_menu, p);
	} else {
		// For each menu that represents plugin's class (possibly several)
		for (iterator i = range.first; i != range.second ; ++i) {
			add_plugin_to_menu(i->second, p);
		}
	}
}

size_t
PluginMenu::build_plugin_class_menu(Gtk::Menu*               menu,
                                    const LilvPluginClass*   plugin_class,
                                    const LilvPluginClasses* classes,
                                    const LV2Children&       children,
                                    std::set<const char*>&   ancestors)
{
	size_t          num_items     = 0;
	const LilvNode* class_uri     = lilv_plugin_class_get_uri(plugin_class);
	const char*     class_uri_str = lilv_node_as_string(class_uri);

	const std::pair<LV2Children::const_iterator, LV2Children::const_iterator> kids
		= children.equal_range(class_uri_str);

	if (kids.first == children.end())
		return 0;

	// Add submenus
	ancestors.insert(class_uri_str);
	for (LV2Children::const_iterator i = kids.first; i != kids.second; ++i) {
		const LilvPluginClass* c = i->second;
		const char* sub_label_str = lilv_node_as_string(lilv_plugin_class_get_label(c));
		const char* sub_uri_str   = lilv_node_as_string(lilv_plugin_class_get_uri(c));
		if (ancestors.find(sub_uri_str) != ancestors.end()) {
			_world.log().warn(fmt("Infinite LV2 class recursion: %1% <: %2%\n")
			                  % class_uri_str % sub_uri_str);
			return 0;
		}

		Gtk::Menu_Helpers::MenuElem menu_elem = Gtk::Menu_Helpers::MenuElem(
			std::string("_") + sub_label_str);
		menu->items().push_back(menu_elem);
		Gtk::MenuItem* menu_item = &(menu->items().back());

		Gtk::Menu* submenu = new Gtk::Menu();
		menu_item->set_submenu(*submenu);

		size_t num_child_items = build_plugin_class_menu(
			submenu, c, classes, children, ancestors);

		_class_menus.insert(std::make_pair(sub_uri_str, MenuRecord(menu_item, submenu)));
		if (num_child_items == 0) {
			menu_item->hide();
		}

		++num_items;
	}
	ancestors.erase(class_uri_str);

	return num_items;
}

void
PluginMenu::add_plugin_to_menu(MenuRecord& menu, SPtr<Client::PluginModel> p)
{
	const URIs& uris        = _world.uris();
	LilvWorld*  lworld      = _world.lilv_world();
	LilvNode*   ingen_Graph = lilv_new_uri(lworld, uris.ingen_Graph.c_str());
	LilvNode*   rdf_type    = lilv_new_uri(lworld, uris.rdf_type.c_str());

	bool is_graph = lilv_world_ask(lworld,
	                               lilv_plugin_get_uri(p->lilv_plugin()),
	                               rdf_type,
	                               ingen_Graph);

	menu.menu->items().push_back(
		Gtk::Menu_Helpers::MenuElem(
			std::string("_") + p->human_name() + (is_graph ? " ⚙" : ""),
			sigc::bind(sigc::mem_fun(this, &PluginMenu::load_plugin), p)));

	if (!menu.item->is_visible()) {
		menu.item->show();
	}

	lilv_node_free(rdf_type);
	lilv_node_free(ingen_Graph);
}

void
PluginMenu::load_plugin(WPtr<Client::PluginModel> weak_plugin)
{
	signal_load_plugin.emit(weak_plugin);
}

} // namespace GUI
} // namespace Ingen
