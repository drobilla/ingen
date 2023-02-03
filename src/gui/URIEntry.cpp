/*
  This file is part of Ingen.
  Copyright 2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "URIEntry.hpp"

#include "App.hpp"
#include "RDFS.hpp"

#include "ingen/World.hpp"

#include <gdk/gdk.h>
#include <glibmm/helperlist.h>
#include <gtkmm/button.h>
#include <gtkmm/menu.h>
#include <gtkmm/menu_elems.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/menushell.h>
#include <gtkmm/object.h>
#include <sigc++/adaptors/bind.h>
#include <sigc++/functors/mem_fun.h>

#include <map>
#include <utility>

namespace ingen::gui {

URIEntry::URIEntry(App* app, std::set<URI> types, const std::string& value)
    : Gtk::HBox(false, 4)
    , _app(app)
    , _types(std::move(types))
    , _menu_button(Gtk::manage(new Gtk::Button("≡")))
    , _entry(Gtk::manage(new Gtk::Entry()))
{
	pack_start(*_entry, true, true);
	pack_start(*_menu_button, false, true);

	_entry->set_text(value);

	_menu_button->signal_event().connect(
		sigc::mem_fun(this, &URIEntry::menu_button_event));
}

Gtk::Menu*
URIEntry::build_value_menu()
{
	World&     world  = _app->world();
	LilvWorld* lworld = world.lilv_world();
	auto*      menu   = new Gtk::Menu();

	LilvNode* owl_onDatatype  = lilv_new_uri(lworld, LILV_NS_OWL "onDatatype");
	LilvNode* rdf_type        = lilv_new_uri(lworld, LILV_NS_RDF "type");
	LilvNode* rdfs_Class      = lilv_new_uri(lworld, LILV_NS_RDFS "Class");
	LilvNode* rdfs_Datatype   = lilv_new_uri(lworld, LILV_NS_RDFS "Datatype");
	LilvNode* rdfs_subClassOf = lilv_new_uri(lworld, LILV_NS_RDFS "subClassOf");

	const rdfs::Objects values = rdfs::instances(world, _types);

	for (const auto& v : values) {
		const LilvNode* inst  = lilv_new_uri(lworld, v.second.c_str());
		std::string     label = v.first;
		if (label.empty()) {
			// No label, show raw URI
			label = lilv_node_as_string(inst);
		}

		if (lilv_world_ask(world.lilv_world(), inst, rdf_type, rdfs_Class) ||
		    lilv_world_ask(world.lilv_world(), inst, rdf_type, rdfs_Datatype)) {
			// This value is a class or datatype...
			if (!lilv_world_ask(lworld, inst, rdfs_subClassOf, nullptr) &&
			    !lilv_world_ask(lworld, inst, owl_onDatatype, nullptr)) {
				// ... which is not a subtype of another, add menu
				add_class_menu_item(menu, inst, label);
			}
		} else {
			// Value is not a class, add item
			menu->items().push_back(
				Gtk::Menu_Helpers::MenuElem(
					std::string("_") + label,
					sigc::bind(sigc::mem_fun(this, &URIEntry::uri_chosen),
					           std::string(lilv_node_as_uri(inst)))));
			_app->set_tooltip(&menu->items().back(), inst);
		}
	}

	lilv_node_free(owl_onDatatype);
	lilv_node_free(rdf_type);
	lilv_node_free(rdfs_Class);
	lilv_node_free(rdfs_Datatype);
	lilv_node_free(rdfs_subClassOf);

	return menu;
}

Gtk::Menu*
URIEntry::build_subclass_menu(const LilvNode* klass)
{
	World&     world  = _app->world();
	LilvWorld* lworld = world.lilv_world();

	LilvNode* owl_onDatatype  = lilv_new_uri(lworld, LILV_NS_OWL "onDatatype");
	LilvNode* rdfs_subClassOf = lilv_new_uri(lworld, LILV_NS_RDFS "subClassOf");

	LilvNodes* subclasses = lilv_world_find_nodes(
		lworld, nullptr, rdfs_subClassOf, klass);
	LilvNodes* subtypes = lilv_world_find_nodes(
		lworld, nullptr, owl_onDatatype, klass);

	if (lilv_nodes_size(subclasses) == 0 && lilv_nodes_size(subtypes) == 0) {
		return nullptr;
	}

	auto* menu = new Gtk::Menu();

	// Add "header" item for choosing this class itself
	add_leaf_menu_item(menu, klass, rdfs::label(world, klass));
	menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

	// Put subclasses/types in a map keyed by label (to sort menu)
	std::map<std::string, const LilvNode*> entries;
	LILV_FOREACH (nodes, s, subclasses) {
		const LilvNode* node = lilv_nodes_get(subclasses, s);
		entries.emplace(rdfs::label(world, node), node);
	}
	LILV_FOREACH (nodes, s, subtypes) {
		const LilvNode* node = lilv_nodes_get(subtypes, s);
		entries.emplace(rdfs::label(world, node), node);
	}

	// Add an item (possibly with a submenu) for each subclass/type
	for (const auto& e : entries) {
		add_class_menu_item(menu, e.second, e.first);
	}

	lilv_nodes_free(subtypes);
	lilv_nodes_free(subclasses);
	lilv_node_free(rdfs_subClassOf);
	lilv_node_free(owl_onDatatype);

	return menu;
}

void
URIEntry::add_leaf_menu_item(Gtk::Menu*         menu,
                             const LilvNode*    node,
                             const std::string& label)
{
	menu->items().push_back(
		Gtk::Menu_Helpers::MenuElem(
			std::string("_") + label,
			sigc::bind(sigc::mem_fun(this, &URIEntry::uri_chosen),
			           std::string(lilv_node_as_uri(node)))));

	_app->set_tooltip(&menu->items().back(), node);
}

void
URIEntry::add_class_menu_item(Gtk::Menu*         menu,
                              const LilvNode*    klass,
                              const std::string& label)
{
	Gtk::Menu* submenu = build_subclass_menu(klass);

	if (submenu) {
		menu->items().push_back(Gtk::Menu_Helpers::MenuElem(label));
		menu->items().back().set_submenu(*Gtk::manage(submenu));
	} else {
		add_leaf_menu_item(menu, klass, label);
	}

	_app->set_tooltip(&menu->items().back(), klass);
}

void
URIEntry::uri_chosen(const std::string& uri)
{
	_entry->set_text(uri);
}

bool
URIEntry::menu_button_event(GdkEvent* ev)
{
	if (ev->type != GDK_BUTTON_PRESS) {
		return false;
	}

	Gtk::Menu* menu = Gtk::manage(build_value_menu());
	menu->popup(ev->button.button, ev->button.time);

	return true;
}

} // namespace ingen::gui
