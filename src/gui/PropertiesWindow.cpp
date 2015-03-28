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

#include <algorithm>
#include <cassert>
#include <set>

#include <gtkmm/label.h>
#include <gtkmm/spinbutton.h>

#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/World.hpp"
#include "ingen/client/BlockModel.hpp"
#include "ingen/client/PluginModel.hpp"

#include "App.hpp"
#include "PropertiesWindow.hpp"
#include "RDFS.hpp"

using namespace std;

namespace Ingen {

using namespace Client;

namespace GUI {

typedef std::set<Raul::URI> URISet;

PropertiesWindow::PropertiesWindow(BaseObjectType*                   cobject,
                                   const Glib::RefPtr<Gtk::Builder>& xml)
	: Window(cobject)
{
	xml->get_widget("properties_vbox", _vbox);
	xml->get_widget("properties_scrolledwindow", _scrolledwindow);
	xml->get_widget("properties_table", _table);
	xml->get_widget("properties_key_combo", _key_combo);
	xml->get_widget("properties_value_entry", _value_entry);
	xml->get_widget("properties_value_button", _value_button);
	xml->get_widget("properties_add_button", _add_button);
	xml->get_widget("properties_cancel_button", _cancel_button);
	xml->get_widget("properties_apply_button", _apply_button);
	xml->get_widget("properties_ok_button", _ok_button);

	_key_store = Gtk::ListStore::create(_combo_columns);
	_key_combo->set_model(_key_store);
	_key_combo->pack_start(_combo_columns.label_col);

	_key_combo->signal_changed().connect(
		sigc::mem_fun(this, &PropertiesWindow::key_changed));

	_value_button->signal_event().connect(
		sigc::mem_fun(this, &PropertiesWindow::value_clicked));

	_add_button->signal_clicked().connect(
		sigc::mem_fun(this, &PropertiesWindow::add_clicked));

	_cancel_button->signal_clicked().connect(
		sigc::mem_fun(this, &PropertiesWindow::cancel_clicked));

	_apply_button->signal_clicked().connect(
		sigc::mem_fun(this, &PropertiesWindow::apply_clicked));

	_ok_button->signal_clicked().connect(
		sigc::mem_fun(this, &PropertiesWindow::ok_clicked));
}

void
PropertiesWindow::reset()
{
	_property_connection.disconnect();
	_property_removed_connection.disconnect();

	_key_store->clear();
	_value_entry->set_text("");
	_records.clear();

	_model.reset();

	_table->children().clear();
	_table->resize(1, 3);
	_table->property_n_rows() = 1;
}

void
PropertiesWindow::present(SPtr<const ObjectModel> model)
{
	set_object(model);
	Gtk::Window::present();
}

void
PropertiesWindow::add_property(const Raul::URI& uri, const Atom& value)
{
	World* world = _app->world();

	const unsigned n_rows = _table->property_n_rows() + 1;
	_table->property_n_rows() = n_rows;

	// Column 0: Property
	LilvNode*     prop     = lilv_new_uri(world->lilv_world(), uri.c_str());
	Glib::ustring lab_text = RDFS::label(world, prop);
	if (lab_text.empty()) {
		lab_text = world->rdf_world()->prefixes().qualify(uri);
	}
	lab_text = Glib::ustring("<a href=\"") + uri + "\">"
		+ lab_text + "</a>";
	Gtk::Label* lab = manage(new Gtk::Label(lab_text, 1.0, 0.5));
	lab->set_use_markup(true);
	set_tooltip(lab, prop);
	_table->attach(*lab, 0, 1, n_rows, n_rows + 1,
	               Gtk::FILL|Gtk::SHRINK, Gtk::SHRINK);

	// Column 1: Value
	Gtk::Alignment*   align      = manage(new Gtk::Alignment(0.0, 0.5, 1.0, 0.0));
	Gtk::CheckButton* present    = manage(new Gtk::CheckButton());
	Gtk::Widget*      val_widget = create_value_widget(uri, value);
	present->set_active(true);
	if (val_widget) {
		align->add(*val_widget);
		set_tooltip(val_widget, prop);
	}
	_table->attach(*align, 1, 2, n_rows, n_rows + 1,
	               Gtk::FILL|Gtk::EXPAND, Gtk::SHRINK);
	_table->attach(*present, 2, 3, n_rows, n_rows + 1,
	               Gtk::FILL, Gtk::SHRINK);
	_records.insert(make_pair(uri, Record(value, align, n_rows, present)));

	lilv_node_free(prop);
}

/** Set the node this window is associated with.
 * This function MUST be called before using this object in any way.
 */
void
PropertiesWindow::set_object(SPtr<const ObjectModel> model)
{
	reset();
	_model = model;

	set_title(model->path() + " Properties - Ingen");

	World* world = _app->world();

	LilvNode* rdfs_range = lilv_new_uri(
		world->lilv_world(), LILV_NS_RDFS "range");
	LilvNode* rdf_type = lilv_new_uri(
		world->lilv_world(), LILV_NS_RDF "type");

	// Populate key combo
	const URISet props = RDFS::properties(world, model);
	for (const auto& p : props) {
		LilvNode*           prop  = lilv_new_uri(world->lilv_world(), p.c_str());
		const Glib::ustring label = RDFS::label(world, prop);
		if (label.empty()) {
			continue;
		}

		// Get all classes in the range of this property (including sub-classes)
		LilvNodes* range = lilv_world_find_nodes(
			world->lilv_world(), prop, rdfs_range, NULL);
		URISet ranges;
		LILV_FOREACH(nodes, r, range) {
			ranges.insert(Raul::URI(lilv_node_as_string(lilv_nodes_get(range, r))));
		}
		RDFS::classes(world, ranges, false);

		bool show = false;
		for (const auto& r : ranges) {
			LilvNode*  range   = lilv_new_uri(world->lilv_world(), r.c_str());
			LilvNodes* objects = lilv_world_find_nodes(
				world->lilv_world(), NULL, rdf_type, range);

			show = lilv_nodes_get_first(objects);

			lilv_nodes_free(objects);
			lilv_node_free(range);
			if (show) {
				break;  // At least one appliable object
			}
		}

		if (show || ranges.empty()) {
			Gtk::ListStore::iterator ki  = _key_store->append();
			Gtk::ListStore::Row      row = *ki;
			row[_combo_columns.uri_col]   = p;
			row[_combo_columns.label_col] = label;
		}

		lilv_node_free(prop);
	}

	lilv_node_free(rdfs_range);
	lilv_node_free(rdf_type);

	for (const auto& p : model->properties()) {
		add_property(p.first, p.second);
	}

	_table->show_all();

	_property_connection = model->signal_property().connect(
		sigc::mem_fun(this, &PropertiesWindow::property_changed));
	_property_removed_connection = model->signal_property_removed().connect(
		sigc::mem_fun(this, &PropertiesWindow::property_removed));
}

Gtk::Widget*
PropertiesWindow::create_value_widget(const Raul::URI& uri, const Atom& value)
{
	Ingen::Forge& forge = _app->forge();
	if (value.type() == forge.Int) {
		Gtk::SpinButton* widget = manage(new Gtk::SpinButton(0.0, 0));
		widget->property_numeric() = true;
		widget->set_range(INT_MIN, INT_MAX);
		widget->set_increments(1, 10);
		widget->set_value(value.get<int32_t>());
		widget->signal_value_changed().connect(
			sigc::bind(sigc::mem_fun(this, &PropertiesWindow::value_edited),
			           uri));
		return widget;
	} else if (value.type() == forge.Float) {
		Gtk::SpinButton* widget = manage(new Gtk::SpinButton(0.0, 4));
		widget->property_numeric() = true;
		widget->set_snap_to_ticks(false);
		widget->set_range(-FLT_MAX, FLT_MAX);
		widget->set_value(value.get<float>());
		widget->set_increments(0.1, 1.0);
		widget->signal_value_changed().connect(
			sigc::bind(sigc::mem_fun(this, &PropertiesWindow::value_edited),
			           uri));
		return widget;
	} else if (value.type() == forge.Bool) {
		Gtk::CheckButton* widget = manage(new Gtk::CheckButton());
		widget->set_active(value.get<int32_t>());
		widget->signal_toggled().connect(
			sigc::bind(sigc::mem_fun(this, &PropertiesWindow::value_edited),
			           uri));
		return widget;
	} else if (value.type() == forge.URI) {
		Gtk::Entry* widget = manage(new Gtk::Entry());
		widget->set_text(value.ptr<char>());
		widget->signal_changed().connect(
			sigc::bind(sigc::mem_fun(this, &PropertiesWindow::value_edited),
			           uri));
		return widget;
	} else if (value.type() == forge.URID) {
		const char* val_uri = _app->world()->uri_map().unmap_uri(value.get<int32_t>());
		Gtk::Entry* widget = manage(new Gtk::Entry());
		if (val_uri) {
			widget->set_text(val_uri);
		}
		widget->signal_changed().connect(
			sigc::bind(sigc::mem_fun(this, &PropertiesWindow::value_edited),
			           uri));
		return widget;
	} else if (value.type() == forge.String) {
		Gtk::Entry* widget = manage(new Gtk::Entry());
		widget->set_text(value.ptr<char>());
		widget->signal_changed().connect(
			sigc::bind(sigc::mem_fun(this, &PropertiesWindow::value_edited),
			           uri));
		return widget;
	}

	_app->log().error(fmt("Unable to create widget for value %1% type %2%\n")
	                  % forge.str(value) % value.type());
	return NULL;
}

void
PropertiesWindow::on_show()
{
	static const int WIN_PAD  = 64;
	static const int VBOX_PAD = 16;

	int width  = 0;
	int height = 0;
	Gtk::Requisition req;

	for (const auto& c : _vbox->children()) {
		req     = c.get_widget()->size_request();
		width   = std::max(width, req.width);
		height += req.height + VBOX_PAD;
	}

	req     = _table->size_request();
	width   = 1.6 * std::max(width, req.width + 128);
	height += req.height;

	set_default_size(width + WIN_PAD, height + WIN_PAD);
	resize(width + WIN_PAD, height + WIN_PAD);
	Gtk::Window::on_show();
}

void
PropertiesWindow::property_changed(const Raul::URI& predicate,
                                   const Atom&      value)
{
	Records::iterator r = _records.find(predicate);
	if (r == _records.end()) {
		add_property(predicate, value);
		_table->show_all();
		return;
	}

	Record&      record       = r->second;
	Gtk::Widget* value_widget = create_value_widget(predicate, value);

	record.value_widget->remove();
	if (value_widget) {
		record.value_widget->add(*value_widget);
		value_widget->show();
	}
	record.value = value;
}

void
PropertiesWindow::property_removed(const Raul::URI& predicate,
                                   const Atom&      value)
{
	// Bleh, there doesn't seem to be an easy way to remove a Gtk::Table row...
	_records.clear();
	_table->children().clear();
	_table->resize(1, 3);
	_table->property_n_rows() = 1;

	for (const auto& p : _model->properties()) {
		add_property(p.first, p.second);
	}
	_table->show_all();
}

void
PropertiesWindow::value_edited(const Raul::URI& predicate)
{
	Records::iterator r = _records.find(predicate);
	if (r == _records.end()) {
		_app->log().error(fmt("Unknown property `%1%' edited\n")
		                  % predicate);
		return;
	}

	Forge&   forge  = _app->forge();
	Record&  record = r->second;
	LV2_URID type   = record.value.type();
	if (type == forge.Int) {
		Gtk::SpinButton* widget = dynamic_cast<Gtk::SpinButton*>(record.value_widget->get_child());
		if (!widget) goto bad_type;
		record.value = _app->forge().make(widget->get_value_as_int());
	} else if (type == forge.Float) {
		Gtk::SpinButton* widget = dynamic_cast<Gtk::SpinButton*>(record.value_widget->get_child());
		if (!widget) goto bad_type;
		record.value = _app->forge().make(static_cast<float>(widget->get_value()));
	} else if (type == forge.Bool) {
		Gtk::CheckButton* widget = dynamic_cast<Gtk::CheckButton*>(record.value_widget->get_child());
		if (!widget) goto bad_type;
		record.value = _app->forge().make(widget->get_active());
	} else if (type == forge.URI) {
		Gtk::Entry* widget = dynamic_cast<Gtk::Entry*>(record.value_widget->get_child());
		if (!widget) goto bad_type;
		record.value = _app->forge().alloc_uri(widget->get_text());
	} else if (type == forge.String) {
		Gtk::Entry* widget = dynamic_cast<Gtk::Entry*>(record.value_widget->get_child());
		if (!widget) goto bad_type;
		record.value = _app->forge().alloc(widget->get_text());
	}

	return;

bad_type:
	_app->log().error(fmt("Property `%1%' value widget has wrong type\n")
	                  % predicate);
	return;
}

std::string
PropertiesWindow::active_property() const
{
	const Gtk::ListStore::iterator iter = _key_combo->get_active();
	if (!iter) {
		return "";
	}

	Glib::ustring prop_uri = (*iter)[_combo_columns.uri_col];
	return prop_uri;
}

void
PropertiesWindow::key_changed()
{
	/* TODO: Clear value?  Build new selector widget, once one for things other than
	   URIs actually exists.  At the moment, clicking the menu button will
	   generate the appropriate menu anyway. */
}

void
PropertiesWindow::set_tooltip(Gtk::Widget* widget, const LilvNode* node)
{
	const Glib::ustring comment = RDFS::comment(_app->world(), node);
	if (!comment.empty()) {
		widget->set_tooltip_text(comment);
	}
}

void
PropertiesWindow::add_class_menu_item(Gtk::Menu* menu, const LilvNode* klass)
{
	const Glib::ustring label   = RDFS::label(_app->world(), klass);
	Gtk::Menu*          submenu = build_subclass_menu(klass);

	if (submenu) {
		menu->items().push_back(Gtk::Menu_Helpers::MenuElem(label));
		Gtk::MenuItem* menu_item = &(menu->items().back());
		set_tooltip(menu_item, klass);
		menu_item->set_submenu(*submenu);
	} else {
		menu->items().push_back(
			Gtk::Menu_Helpers::MenuElem(
				label,
				sigc::bind(sigc::mem_fun(this, &PropertiesWindow::uri_chosen),
				           std::string(lilv_node_as_uri(klass)))));
	}
	set_tooltip(&(menu->items().back()), klass);
}

Gtk::Menu*
PropertiesWindow::build_subclass_menu(const LilvNode* klass)
{
	World* world = _app->world();

	LilvNode* rdfs_subClassOf = lilv_new_uri(
		world->lilv_world(), LILV_NS_RDFS "subClassOf");
	LilvNodes* subclasses = lilv_world_find_nodes(
		world->lilv_world(), NULL, rdfs_subClassOf, klass);

	if (lilv_nodes_size(subclasses) == 0) {
		return NULL;
	}

	const Glib::ustring label = RDFS::label(world, klass);
	Gtk::Menu*          menu  = new Gtk::Menu();

	// Add "header" item for choosing this class itself
	menu->items().push_back(
		Gtk::Menu_Helpers::MenuElem(
			label,
			sigc::bind(sigc::mem_fun(this, &PropertiesWindow::uri_chosen),
			           std::string(lilv_node_as_uri(klass)))));
	menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());
	set_tooltip(&(menu->items().back()), klass);

	// Add an item (and maybe submenu) for each subclass
	LILV_FOREACH(nodes, s, subclasses) {
		add_class_menu_item(menu, lilv_nodes_get(subclasses, s));
	}
	lilv_nodes_free(subclasses);
	return menu;
}

void
PropertiesWindow::build_value_menu(Gtk::Menu* menu, const LilvNodes* ranges)
{
	World*     world  = _app->world();
	LilvWorld* lworld = world->lilv_world();

	LilvNode* rdf_type        = lilv_new_uri(lworld, LILV_NS_RDF "type");
	LilvNode* rdfs_Class      = lilv_new_uri(lworld, LILV_NS_RDFS "Class");
	LilvNode* rdfs_subClassOf = lilv_new_uri(lworld, LILV_NS_RDFS "subClassOf");

	LILV_FOREACH(nodes, r, ranges) {
		const LilvNode* klass = lilv_nodes_get(ranges, r);
		if (!lilv_node_is_uri(klass)) {
			continue;
		}
		const char* uri = lilv_node_as_string(klass);

		// Add items for instances of this class
		RDFS::URISet ranges_uris;
		ranges_uris.insert(Raul::URI(uri));
		RDFS::Objects values = RDFS::instances(world, ranges_uris);
		for (const auto& v : values) {
			const LilvNode* inst  = lilv_new_uri(lworld, v.first.c_str());
			Glib::ustring   label = RDFS::label(world, inst);
			if (label.empty()) {
				label = lilv_node_as_string(inst);
			}

			if (lilv_world_ask(world->lilv_world(), inst, rdf_type, rdfs_Class)) {
				if (!lilv_world_ask(lworld, inst, rdfs_subClassOf, NULL) ||
				    lilv_world_ask(lworld, inst, rdfs_subClassOf, inst)) {
					add_class_menu_item(menu, inst);
				}
			} else {
				menu->items().push_back(
					Gtk::Menu_Helpers::MenuElem(
						label,
						sigc::bind(sigc::mem_fun(this, &PropertiesWindow::uri_chosen),
						           std::string(lilv_node_as_uri(inst)))));
				set_tooltip(&(menu->items().back()), inst);
			}
		}
	}
}

void
PropertiesWindow::uri_chosen(const std::string& uri)
{
	_value_entry->set_text(uri);
}

bool
PropertiesWindow::value_clicked(GdkEvent* ev)
{
	if (ev->type != GDK_BUTTON_PRESS) {
		return false;
	}

	// Get currently selected property (key) to add
	const std::string prop_uri = active_property();
	if (prop_uri.empty()) {
		return false;
	}

	World* world = _app->world();

	LilvNode* rdfs_range = lilv_new_uri(
		world->lilv_world(), LILV_NS_RDFS "range");

	LilvNode*  prop   = lilv_new_uri(world->lilv_world(), prop_uri.c_str());
	LilvNodes* ranges = lilv_world_find_nodes(
		world->lilv_world(), prop, rdfs_range, NULL);

	Gtk::Menu* menu = new Gtk::Menu();
	build_value_menu(menu, ranges);
	menu->popup(ev->button.button, ev->button.time);
	return true;
}

void
PropertiesWindow::add_clicked()
{
	if (!_key_combo->get_active() || _value_entry->get_text().empty()) {
		return;
	}

	const Gtk::ListStore::Row krow      = *(_key_combo->get_active());
	const Glib::ustring       key_uri   = krow[_combo_columns.uri_col];
	const Glib::ustring       value_uri = _value_entry->get_text();

	Atom value = _app->forge().alloc_uri(value_uri);

	Resource::Properties properties;
	properties.insert(make_pair(Raul::URI(key_uri.c_str()),
	                            Resource::Property(value)));
	_app->interface()->put(_model->uri(), properties);
}

void
PropertiesWindow::cancel_clicked()
{
	reset();
	Gtk::Window::hide();
}

void
PropertiesWindow::apply_clicked()
{
	Resource::Properties remove;
	Resource::Properties add;
	for (const auto& r : _records) {
		const Raul::URI& uri    = r.first;
		const Record&    record = r.second;
		if (record.present_button->get_active()) {
			if (!_model->has_property(uri, record.value)) {
				add.insert(make_pair(uri, record.value));
			}
		} else {
			remove.insert(make_pair(uri, record.value));
		}
	}

	if (remove.empty()) {
		_app->interface()->put(_model->uri(), add);
	} else {
		_app->interface()->delta(_model->uri(), remove, add);
	}
}

void
PropertiesWindow::ok_clicked()
{
	apply_clicked();
	Gtk::Window::hide();
}

} // namespace GUI
} // namespace Ingen
