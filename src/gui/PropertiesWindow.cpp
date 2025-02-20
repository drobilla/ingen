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

#include "PropertiesWindow.hpp"

#include "App.hpp"
#include "RDFS.hpp"
#include "URIEntry.hpp"
#include "Window.hpp"

#include <ingen/Atom.hpp>
#include <ingen/Forge.hpp>
#include <ingen/Interface.hpp>
#include <ingen/Log.hpp>
#include <ingen/Properties.hpp>
#include <ingen/URI.hpp>
#include <ingen/URIMap.hpp>
#include <ingen/URIs.hpp>
#include <ingen/World.hpp>
#include <ingen/client/ObjectModel.hpp>
#include <lilv/lilv.h>
#include <lv2/urid/urid.h>
#include <raul/Path.hpp>
#include <sord/sordmm.hpp>

#include <glibmm/containers.h>
#include <glibmm/propertyproxy.h>
#include <glibmm/ustring.h>
#include <gtk/gtk.h>
#include <gtkmm/alignment.h>
#include <gtkmm/bin.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/combobox.h>
#include <gtkmm/entry.h>
#include <gtkmm/enums.h>
#include <gtkmm/label.h>
#include <gtkmm/object.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/table.h>
#include <gtkmm/treeiter.h>
#include <gtkmm/widget.h>
#include <gtkmm/window.h>
#include <sigc++/adaptors/bind.h>
#include <sigc++/functors/mem_fun.h>
#include <sigc++/signal.h>

#include <algorithm>
#include <cfloat>
#include <climits>
#include <cstdint>
#include <memory>
#include <set>
#include <utility>

namespace ingen {

using client::ObjectModel;

namespace gui {

using URISet = std::set<URI>;

PropertiesWindow::PropertiesWindow(BaseObjectType*                   cobject,
                                   const Glib::RefPtr<Gtk::Builder>& xml)
	: Window(cobject)
{
	xml->get_widget("properties_vbox", _vbox);
	xml->get_widget("properties_scrolledwindow", _scrolledwindow);
	xml->get_widget("properties_table", _table);
	xml->get_widget("properties_key_combo", _key_combo);
	xml->get_widget("properties_value_bin", _value_bin);
	xml->get_widget("properties_add_button", _add_button);
	xml->get_widget("properties_cancel_button", _cancel_button);
	xml->get_widget("properties_apply_button", _apply_button);
	xml->get_widget("properties_ok_button", _ok_button);

	_key_store = Gtk::ListStore::create(_combo_columns);
	_key_combo->set_model(_key_store);
	_key_combo->pack_start(_combo_columns.label_col);

	_key_combo->signal_changed().connect(
		sigc::mem_fun(this, &PropertiesWindow::key_changed));

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
	_records.clear();

	_model.reset();

	_table->children().clear();
	_table->resize(1, 3);
	_table->property_n_rows() = 1;
}

void
PropertiesWindow::present(const std::shared_ptr<const ObjectModel>& model)
{
	set_object(model);
	Gtk::Window::present();
}

void
PropertiesWindow::add_property(const URI& key, const Atom& value)
{
	World& world = _app->world();

	const unsigned n_rows = _table->property_n_rows() + 1;
	_table->property_n_rows() = n_rows;

	// Column 0: Property
	LilvNode*   prop = lilv_new_uri(world.lilv_world(), key.c_str());
	std::string name = rdfs::label(world, prop);
	if (name.empty()) {
		name = world.rdf_world()->prefixes().qualify(key);
	}

	auto* label = new Gtk::Label(std::string("<a href=\"") + key.string() +
	                                 "\">" + name + "</a>",
	                             1.0,
	                             0.5);

	label->set_use_markup(true);
	_app->set_tooltip(label, prop);
	_table->attach(*Gtk::manage(label), 0, 1, n_rows, n_rows + 1,
	               Gtk::FILL|Gtk::SHRINK, Gtk::SHRINK);

	// Column 1: Value
	Gtk::Alignment*   align      = manage(new Gtk::Alignment(0.0, 0.5, 1.0, 1.0));
	Gtk::CheckButton* present    = manage(new Gtk::CheckButton());
	const char*       type       = _app->world().uri_map().unmap_uri(value.type());
	Gtk::Widget*      val_widget = create_value_widget(key, type, value);

	present->set_active();
	if (val_widget) {
		align->add(*Gtk::manage(val_widget));
		_app->set_tooltip(val_widget, prop);
	}

	_table->attach(*align, 1, 2, n_rows, n_rows + 1,
	               Gtk::FILL|Gtk::EXPAND, Gtk::SHRINK);
	_table->attach(*present, 2, 3, n_rows, n_rows + 1,
	               Gtk::FILL, Gtk::SHRINK);
	_records.emplace(key, Record(value, align, n_rows, present));
	_table->show_all();

	lilv_node_free(prop);
}

bool
PropertiesWindow::datatype_supported(const rdfs::URISet& types,
                                     URI*                widget_type)
{
	if (types.find(_app->uris().atom_Int) != types.end()) {
		*widget_type = _app->uris().atom_Int;
		return true;
	}

	if (types.find(_app->uris().atom_Float) != types.end()) {
		*widget_type = _app->uris().atom_Float;
		return true;
	}

	if (types.find(_app->uris().atom_Bool) != types.end()) {
		*widget_type = _app->uris().atom_Bool;
		return true;
	}

	if (types.find(_app->uris().atom_String) != types.end()) {
		*widget_type = _app->uris().atom_String;
		return true;
	}

	if (types.find(_app->uris().atom_URID) != types.end()) {
		*widget_type = _app->uris().atom_URID;
		return true;
	}

	return false;
}

bool
PropertiesWindow::class_supported(const rdfs::URISet& types)
{
	World&    world    = _app->world();
	LilvNode* rdf_type = lilv_new_uri(world.lilv_world(), LILV_NS_RDF "type");

	for (const auto& t : types) {
		LilvNode*   range     = lilv_new_uri(world.lilv_world(), t.c_str());
		LilvNodes*  instances = lilv_world_find_nodes(
			world.lilv_world(), nullptr, rdf_type, range);

		const bool has_instance = (lilv_nodes_size(instances) > 0);

		lilv_nodes_free(instances);
		lilv_node_free(range);
		if (has_instance) {
			lilv_node_free(rdf_type);
			return true;
		}
	}

	lilv_node_free(rdf_type);
	return false;
}

/** Set the node this window is associated with.
 * This function MUST be called before using this object in any way.
 */
void
PropertiesWindow::set_object(const std::shared_ptr<const ObjectModel>& model)
{
	reset();
	_model = model;

	set_title(model->path() + " Properties - Ingen");

	World& world = _app->world();

	LilvNode* rdf_type = lilv_new_uri(
		world.lilv_world(), LILV_NS_RDF "type");
	LilvNode* rdfs_DataType = lilv_new_uri(
		world.lilv_world(), LILV_NS_RDFS "Datatype");

	// Populate key combo
	const URISet               props = rdfs::properties(world, model);
	std::map<std::string, URI> entries;
	for (const auto& p : props) {
		LilvNode*         prop   = lilv_new_uri(world.lilv_world(), p.c_str());
		const std::string label  = rdfs::label(world, prop);
		URISet            ranges = rdfs::range(world, prop, true);

		lilv_node_free(prop);
		if (label.empty() || ranges.empty()) {
			// Property has no label or range, can't show a widget for it
			continue;
		}

		LilvNode* range = lilv_new_uri(world.lilv_world(), (*ranges.begin()).c_str());
		if (rdfs::is_a(world, range, rdfs_DataType)) {
			// Range is a datatype, show if type or any subtype is supported
			rdfs::datatypes(_app->world(), ranges, false);
			URI widget_type("urn:nothing");
			if (datatype_supported(ranges, &widget_type)) {
				entries.emplace(label, p);
			}
		} else {
			// Range is presumably a class, show if any instances are known
			if (class_supported(ranges)) {
				entries.emplace(label, p);
			}
		}
	}

	for (const auto& e : entries) {
		auto ki  = _key_store->append();
		auto row = *ki;

		row[_combo_columns.uri_col]   = e.second.string();
		row[_combo_columns.label_col] = e.first;
	}

	lilv_node_free(rdfs_DataType);
	lilv_node_free(rdf_type);

	for (const auto& p : model->properties()) {
		add_property(p.first, p.second);
	}

	_table->show_all();

	_property_connection = model->signal_property().connect(
		sigc::mem_fun(this, &PropertiesWindow::add_property));
	_property_removed_connection = model->signal_property_removed().connect(
		sigc::mem_fun(this, &PropertiesWindow::remove_property));
}

Gtk::Widget*
PropertiesWindow::create_value_widget(const URI&  key,
                                      const char* type_uri,
                                      const Atom& value)
{
	if (!type_uri || !URI::is_valid(type_uri)) {
		return nullptr;
	}

	URI     type(type_uri);
	ingen::World& world  = _app->world();
	LilvWorld*    lworld = world.lilv_world();

	// See if type is a datatype we support
	std::set<URI> types{type};
	rdfs::datatypes(world, types, false);

	URI  widget_type("urn:nothing");
	const bool supported = datatype_supported(types, &widget_type);
	if (supported) {
		type = widget_type;
		_value_type = world.uri_map().map_uri(type);
	}

	if (type == _app->uris().atom_Int) {
		Gtk::SpinButton* widget = manage(new Gtk::SpinButton(0.0, 0));
		widget->property_numeric() = true;
		widget->set_range(INT_MIN, INT_MAX);
		widget->set_increments(1, 10);
		if (value.is_valid()) {
			widget->set_value(value.get<int32_t>());
		}
		widget->signal_value_changed().connect(
			sigc::bind(sigc::mem_fun(this, &PropertiesWindow::on_change), key));
		return widget;
	}

	if (type == _app->uris().atom_Float) {
		Gtk::SpinButton* widget = manage(new Gtk::SpinButton(0.0, 4));
		widget->property_numeric() = true;
		widget->set_snap_to_ticks(false);
		widget->set_range(-DBL_MAX, DBL_MAX);
		widget->set_increments(0.1, 1.0);
		if (value.is_valid()) {
			widget->set_value(static_cast<double>(value.get<float>()));
		}
		widget->signal_value_changed().connect(
			sigc::bind(sigc::mem_fun(this, &PropertiesWindow::on_change), key));
		return widget;
	}

	if (type == _app->uris().atom_Bool) {
		Gtk::CheckButton* widget = manage(new Gtk::CheckButton());
		if (value.is_valid()) {
			widget->set_active(value.get<int32_t>());
		}
		widget->signal_toggled().connect(
			sigc::bind(sigc::mem_fun(this, &PropertiesWindow::on_change), key));
		return widget;
	}

	if (type == _app->uris().atom_String) {
		Gtk::Entry* widget = manage(new Gtk::Entry());
		if (value.is_valid()) {
			widget->set_text(value.ptr<char>());
		}
		widget->signal_changed().connect(
			sigc::bind(sigc::mem_fun(this, &PropertiesWindow::on_change), key));
		return widget;
	}

	if (type == _app->uris().atom_URID) {
		const char* str = (value.is_valid()
		                   ? world.uri_map().unmap_uri(value.get<int32_t>())
		                   : "");

		LilvNode*    pred   = lilv_new_uri(lworld, key.c_str());
		const URISet ranges = rdfs::range(world, pred, true);
		URIEntry*    widget = manage(new URIEntry(_app, ranges, str ? str : ""));
		widget->signal_changed().connect(
			sigc::bind(sigc::mem_fun(this, &PropertiesWindow::on_change), key));
		lilv_node_free(pred);
		return widget;
	}

	LilvNode*  type_node  = lilv_new_uri(lworld, type.c_str());
	LilvNode*  rdfs_Class = lilv_new_uri(lworld, LILV_NS_RDFS "Class");
	const bool is_class   = rdfs::is_a(world, type_node, rdfs_Class);
	lilv_node_free(rdfs_Class);
	lilv_node_free(type_node);

	if (type == _app->uris().atom_URI ||
	    type == _app->uris().rdfs_Class ||
	    is_class) {
		LilvNode*    pred   = lilv_new_uri(lworld, key.c_str());
		const URISet ranges = rdfs::range(world, pred, true);
		const char*  str    = value.is_valid() ? value.ptr<const char>() : "";
		URIEntry*    widget = manage(new URIEntry(_app, ranges, str));
		widget->signal_changed().connect(
			sigc::bind(sigc::mem_fun(this, &PropertiesWindow::on_change), key));
		lilv_node_free(pred);
		return widget;
	}

	_app->log().error("No widget for value type %1%\n", type);

	return nullptr;
}

void
PropertiesWindow::on_show()
{
	static const int WIN_PAD  = 64;
	static const int VBOX_PAD = 16;

	int width  = 0;
	int height = 0;

	for (const auto& c : _vbox->children()) {
		const Gtk::Requisition& req = c.get_widget()->size_request();

		width   = std::max(width, req.width);
		height += req.height + VBOX_PAD;
	}

	const Gtk::Requisition& req = _table->size_request();

	width   = 1.2 * std::max(width, req.width + 128);
	height += req.height;

	set_default_size(width + WIN_PAD, height + WIN_PAD);
	resize(width + WIN_PAD, height + WIN_PAD);
	Gtk::Window::on_show();
}

void
PropertiesWindow::change_property(const URI& key, const Atom& value)
{
	auto r = _records.find(key);
	if (r == _records.end()) {
		add_property(key, value);
		_table->show_all();
		return;
	}

	Record&      record     = r->second;
	const char*  type       = _app->world().uri_map().unmap_uri(value.type());
	Gtk::Widget* val_widget = create_value_widget(key, type, value);

	if (val_widget) {
		record.value_widget->remove();
		record.value_widget->add(*Gtk::manage(val_widget));
		val_widget->show_all();
	}

	record.value = value;
}

void
PropertiesWindow::remove_property(const URI& key, const Atom& value)
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

Atom
PropertiesWindow::get_value(LV2_URID type, Gtk::Widget* value_widget)
{
	const Forge& forge = _app->forge();

	if (type == forge.Int) {
		auto* spin = dynamic_cast<Gtk::SpinButton*>(value_widget);
		if (spin) {
			return _app->forge().make(spin->get_value_as_int());
		}
	} else if (type == forge.Float) {
		auto* spin = dynamic_cast<Gtk::SpinButton*>(value_widget);
		if (spin) {
			return _app->forge().make(static_cast<float>(spin->get_value()));
		}
	} else if (type == forge.Bool) {
		auto* check = dynamic_cast<Gtk::CheckButton*>(value_widget);
		if (check) {
			return _app->forge().make(check->get_active());
		}
	} else if (type == forge.URI || type == forge.URID) {
		auto* uri_entry = dynamic_cast<URIEntry*>(value_widget);
		if (uri_entry) {
			if (URI::is_valid(uri_entry->get_text())) {
				return _app->forge().make_urid(URI(uri_entry->get_text()));
			}

			_app->log().error("Invalid URI <%1%>\n", uri_entry->get_text());
		}
	} else if (type == forge.String) {
		auto* entry = dynamic_cast<Gtk::Entry*>(value_widget);
		if (entry) {
			return _app->forge().alloc(entry->get_text());
		}
	}

	return {};
}

void
PropertiesWindow::on_change(const URI& key)
{
	auto r = _records.find(key);
	if (r == _records.end()) {
		return;
	}

	Record&    record = r->second;
	const Atom value  = get_value(record.value.type(),
	                              record.value_widget->get_child());

	if (value.is_valid()) {
		record.value = value;
	} else {
		_app->log().error("Failed to get `%1%' value from widget\n", key);
	}
}

std::string
PropertiesWindow::active_key() const
{
	const auto iter = _key_combo->get_active();
	if (!iter) {
		return "";
	}

	const Glib::ustring prop_uri = (*iter)[_combo_columns.uri_col];
	return prop_uri;
}

void
PropertiesWindow::key_changed()
{
	_value_bin->remove();
	if (!_key_combo->get_active()) {
		return;
	}

	LilvWorld*                lworld  = _app->world().lilv_world();
	const Gtk::ListStore::Row key_row = *(_key_combo->get_active());
	const Glib::ustring       key_uri = key_row[_combo_columns.uri_col];
	LilvNode*                 prop    = lilv_new_uri(lworld, key_uri.c_str());

	// Try to create a value widget in the range of this property
	const URISet ranges = rdfs::range(_app->world(), prop, true);
	for (const auto& r : ranges) {
		Gtk::Widget* value_widget = create_value_widget(
			URI(key_uri), r.c_str(), Atom());

		if (value_widget) {
			_add_button->set_sensitive(true);
			_value_bin->remove();
			_value_bin->add(*Gtk::manage(value_widget));
			_value_bin->show_all();
			break;
		}
	}

	lilv_node_free(prop);
}

void
PropertiesWindow::add_clicked()
{
	if (!_key_combo->get_active() || !_value_type || !_value_bin->get_child()) {
		return;
	}

	// Get selected key URI
	const Gtk::ListStore::Row key_row = *(_key_combo->get_active());
	const Glib::ustring       key_uri = key_row[_combo_columns.uri_col];

	// Try to get value from value widget
	const Atom& value = get_value(_value_type, _value_bin->get_child());
	if (value.is_valid()) {
		// Send property to engine
		Properties properties;
		properties.emplace(URI(key_uri.c_str()), Property(value));
		_app->interface()->put(_model->uri(), properties);
	}
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
	Properties remove;
	Properties add;
	for (const auto& r : _records) {
		const URI&    uri    = r.first;
		const Record& record = r.second;
		if (record.present_button->get_active()) {
			if (!_model->has_property(uri, record.value)) {
				add.emplace(uri, record.value);
			}
		} else {
			remove.emplace(uri, record.value);
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

} // namespace gui
} // namespace ingen
