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
#include <string>

#include <gtkmm/label.h>
#include <gtkmm/spinbutton.h>

#include "raul/log.hpp"
#include "ingen/shared/World.hpp"
#include "ingen/client/NodeModel.hpp"
#include "ingen/client/PluginModel.hpp"
#include "App.hpp"
#include "PropertiesWindow.hpp"

#define LOG(s) s << "[PropertiesWindow] "

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Client;
using namespace Shared;

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
	xml->get_widget("properties_value_combo", _value_combo);
	xml->get_widget("properties_add_button", _add_button);
	xml->get_widget("properties_cancel_button", _cancel_button);
	xml->get_widget("properties_apply_button", _apply_button);
	xml->get_widget("properties_ok_button", _ok_button);

	_key_store = Gtk::ListStore::create(_combo_columns);
	_key_combo->set_model(_key_store);
	_key_combo->pack_start(_combo_columns.label_col);

	_value_store = Gtk::ListStore::create(_combo_columns);
	_value_combo->set_model(_value_store);
	_value_combo->pack_start(_combo_columns.label_col);

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
	_table->children().clear();
	_table->resize(1, 3);
	_table->property_n_rows() = 1;

	_records.clear();
	_key_store->clear();
	_value_store->clear();

	_property_connection.disconnect();
	_model.reset();
}

void
PropertiesWindow::present(SharedPtr<const ObjectModel> model)
{
	set_object(model);
	Gtk::Window::present();
}

/** Get all the types which this model is an instance of */
static URISet
get_types(Shared::World* world, SharedPtr<const ObjectModel> model)
{
	typedef Resource::Properties::const_iterator PropIter;
	typedef std::pair<PropIter, PropIter>        PropRange;

	// Start with every rdf:type
	URISet types;
	PropRange range = model->properties().equal_range(world->uris().rdf_type);
	for (PropIter t = range.first; t != range.second; ++t) {
		types.insert(t->second.get_uri());
	}

	LilvNode* rdfs_subClassOf = lilv_new_uri(
		world->lilv_world(), LILV_NS_RDFS "subClassOf");

	/* Add super-classes until no new super-classes are encountered.  Really
	   slow, but won't loop forever even if subClassOf loops exist. */
	unsigned added = 0;
	do {
		added = 0;
		URISet supers;
		for (URISet::iterator t = types.begin(); t != types.end(); ++t) {
			LilvNode*  type = lilv_new_uri(world->lilv_world(), t->c_str());
			LilvNodes* sups = lilv_world_find_nodes(
				world->lilv_world(), type, rdfs_subClassOf, NULL);
			LILV_FOREACH(nodes, s, sups) {
				const LilvNode* super_node = lilv_nodes_get(sups, s);
				if (lilv_node_is_uri(super_node)) {
					Raul::URI super = lilv_node_as_uri(super_node);
					if (!types.count(super)) {
						++added;
						supers.insert(super);
					}
				}
			}
			lilv_nodes_free(sups);
			lilv_node_free(type);
		}
		types.insert(supers.begin(), supers.end());
	} while (added > 0);

	lilv_node_free(rdfs_subClassOf);
	return types;
}

/** Get all the properties with domains appropriate to this model */
static URISet
get_properties(Shared::World* world, SharedPtr<const ObjectModel> model)
{
	URISet properties;
	URISet types = get_types(world, model);

	LilvNode* rdf_type = lilv_new_uri(world->lilv_world(),
	                                  LILV_NS_RDF "type");
	LilvNode* rdf_Property = lilv_new_uri(world->lilv_world(),
	                                      LILV_NS_RDF "Property");
	LilvNode* rdfs_domain = lilv_new_uri(world->lilv_world(),
	                                     LILV_NS_RDFS "domain");

	LilvNodes* props = lilv_world_find_nodes(
		world->lilv_world(), NULL, rdf_type, rdf_Property);
	LILV_FOREACH(nodes, p, props) {
		const LilvNode* prop = lilv_nodes_get(props, p);
		if (lilv_node_is_uri(prop)) {
			LilvNodes* domains = lilv_world_find_nodes(
				world->lilv_world(), prop, rdfs_domain, NULL);
			unsigned n_matching_domains = 0;
			LILV_FOREACH(nodes, d, domains) {
				const LilvNode* domain_node = lilv_nodes_get(domains, d);
				const Raul::URI domain(lilv_node_as_uri(domain_node));
				if (types.count(domain)) {
					++n_matching_domains;
				}
			}

			if (n_matching_domains > 0 &&
			    n_matching_domains == lilv_nodes_size(domains)) {
				properties.insert(lilv_node_as_uri(prop));
			}

			lilv_nodes_free(domains);
		}
	}

	lilv_node_free(rdfs_domain);
	lilv_node_free(rdf_Property);
	lilv_node_free(rdf_type);

	return properties;
}

static Glib::ustring
get_label(Shared::World* world, const LilvNode* node)
{
	LilvNode* rdfs_label = lilv_new_uri(
		world->lilv_world(), LILV_NS_RDFS "label");
	LilvNodes* labels = lilv_world_find_nodes(
		world->lilv_world(), node, rdfs_label, NULL);

	const LilvNode* first = lilv_nodes_get_first(labels);
	Glib::ustring   label = first ? lilv_node_as_string(first) : "";

	lilv_nodes_free(labels);
	lilv_node_free(rdfs_label);
	return label;
}

void
PropertiesWindow::add_property(const Raul::URI& uri, const Raul::Atom& value)
{
	Shared::World* world = _app->world();

	const unsigned n_rows = _table->property_n_rows() + 1;
	_table->property_n_rows() = n_rows;

	// Column 0: Property
	LilvNode* prop = lilv_new_uri(world->lilv_world(), uri.c_str());
	Glib::ustring lab_text = get_label(world, prop);
	if (lab_text.empty()) {
		lab_text = world->rdf_world()->prefixes().qualify(uri.str());
	}
	lab_text = Glib::ustring("<a href=\"") + uri.str() + "\">"
		+ lab_text + "</a>";
	Gtk::Label* lab = manage(new Gtk::Label(lab_text, 1.0, 0.5));
	lab->set_use_markup(true);
	_table->attach(*lab, 0, 1, n_rows, n_rows + 1,
	               Gtk::FILL|Gtk::SHRINK, Gtk::SHRINK);
	lilv_node_free(prop);

	// Column 1: Value
	Gtk::Alignment* align      = manage(new Gtk::Alignment(0.0, 0.5, 1.0, 0.0));
	Gtk::Widget*    val_widget = create_value_widget(uri, value);
	if (val_widget) {
		align->add(*val_widget);
	}
	_table->attach(*align, 1, 2, n_rows, n_rows + 1,
	               Gtk::FILL|Gtk::EXPAND, Gtk::SHRINK);
	_records.insert(make_pair(uri, Record(value, align, n_rows)));
}

/** Set the node this window is associated with.
 * This function MUST be called before using this object in any way.
 */
void
PropertiesWindow::set_object(SharedPtr<const ObjectModel> model)
{
	reset();
	_model = model;

	set_title(model->path().chop_scheme() + " Properties - Ingen");

	Shared::World* world = _app->world();

	LilvNode* rdfs_range = lilv_new_uri(
		world->lilv_world(), LILV_NS_RDFS "range");
	LilvNode* rdf_type = lilv_new_uri(
		world->lilv_world(), LILV_NS_RDF "type");

	// Populate key combo
	const URISet props = get_properties(world, model);
	for (URISet::const_iterator p = props.begin(); p != props.end(); ++p) {
		LilvNode*           prop  = lilv_new_uri(world->lilv_world(), p->c_str());
		const Glib::ustring label = get_label(world, prop);
		if (label.empty()) {
			continue;
		}

		LilvNodes* ranges = lilv_world_find_nodes(
			world->lilv_world(), prop, rdfs_range, NULL);
		LILV_FOREACH(nodes, r, ranges) {
			const LilvNode* range = lilv_nodes_get(ranges, r);
			LilvNodes* objects = lilv_world_find_nodes(
				world->lilv_world(), NULL, rdf_type, range);
			if (lilv_nodes_get_first(objects)) {
				// At least one applicable object, add property to key combo
				Gtk::ListStore::iterator ki  = _key_store->append();
				Gtk::ListStore::Row      row = *ki;
				row[_combo_columns.uri_col]   = p->str();
				row[_combo_columns.label_col] = label;
				break;
			}
			lilv_nodes_free(objects);
		}
		lilv_node_free(prop);
	}

	lilv_node_free(rdfs_range);
	lilv_node_free(rdf_type);

	for (ObjectModel::Properties::const_iterator i = model->properties().begin();
			i != model->properties().end(); ++i) {
		add_property(i->first, i->second);
	}

	_table->show_all();

	_property_connection = model->signal_property().connect(
			sigc::mem_fun(this, &PropertiesWindow::property_changed));
}

Gtk::Widget*
PropertiesWindow::create_value_widget(const Raul::URI& uri, const Raul::Atom& value)
{
	Ingen::Shared::Forge& forge = _app->forge();
	if (value.type() == forge.Int) {
		Gtk::SpinButton* widget = manage(new Gtk::SpinButton(0.0, 0));
		widget->property_numeric() = true;
		widget->set_range(INT_MIN, INT_MAX);
		widget->set_increments(1, 10);
		widget->set_value(value.get_int32());
		widget->signal_value_changed().connect(sigc::bind(
				sigc::mem_fun(this, &PropertiesWindow::value_edited),
				uri));
		return widget;
	} else if (value.type() == forge.Float) {
		Gtk::SpinButton* widget = manage(new Gtk::SpinButton(0.0, 4));
		widget->property_numeric() = true;
		widget->set_snap_to_ticks(false);
		widget->set_range(DBL_MIN, DBL_MAX);
		widget->set_value(value.get_float());
		widget->set_increments(0.1, 1.0);
		widget->signal_value_changed().connect(sigc::bind(
				sigc::mem_fun(this, &PropertiesWindow::value_edited),
				uri));
		return widget;
	} else if (value.type() == forge.Bool) {
		Gtk::CheckButton* widget = manage(new Gtk::CheckButton());
		widget->set_active(value.get_bool());
		widget->signal_toggled().connect(sigc::bind(
				sigc::mem_fun(this, &PropertiesWindow::value_edited),
				uri));
		return widget;
	} else if (value.type() == forge.URI) {
		Gtk::Entry* widget = manage(new Gtk::Entry());
		widget->set_text(value.get_uri());
		widget->signal_changed().connect(sigc::bind(
				sigc::mem_fun(this, &PropertiesWindow::value_edited),
				uri));
		return widget;
	} else if (value.type() == forge.String) {
		Gtk::Entry* widget = manage(new Gtk::Entry());
		widget->set_text(value.get_string());
		widget->signal_changed().connect(sigc::bind(
				sigc::mem_fun(this, &PropertiesWindow::value_edited),
				uri));
		return widget;
	}

	LOG(error) << "Unable to create widget for value " << forge.str(value) << endl;
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

	typedef Gtk::Box_Helpers::BoxList Children;
	for (Children::const_iterator i = _vbox->children().begin(); i != _vbox->children().end(); ++i) {
		req = (*i).get_widget()->size_request();
		if ((*i).get_widget() != _scrolledwindow) {
			width = std::max(width, req.width);
			height += req.height + VBOX_PAD;
		}
	}

	req = _table->size_request();
	width = std::max(width, req.width);
	height += req.height;

	set_default_size(width + WIN_PAD, height + WIN_PAD);
	resize(width + WIN_PAD, height + WIN_PAD);
	Gtk::Window::on_show();
}

void
PropertiesWindow::property_changed(const Raul::URI& predicate, const Raul::Atom& value)
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
PropertiesWindow::value_edited(const Raul::URI& predicate)
{
	Records::iterator r = _records.find(predicate);
	if (r == _records.end()) {
		LOG(error) << "Unknown property `" << predicate << "' edited" << endl;
		return;
	}

	Forge&             forge  = _app->forge();
	Record&            record = r->second;
	Raul::Atom::TypeID type   = record.value.type();
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
	LOG(error) << "Property `" << predicate << "' value widget has wrong type" << endl;
	return;
}

void
PropertiesWindow::key_changed()
{
	Shared::World* world = _app->world();

	const Gtk::ListStore::Row row      = *(_key_combo->get_active());
	Glib::ustring             prop_uri = row[_combo_columns.uri_col];

	if (prop_uri.empty()) {
		return;
	}

	_value_store->clear();

	LilvNode* rdfs_range = lilv_new_uri(
		world->lilv_world(), LILV_NS_RDFS "range");
	LilvNode* rdf_type = lilv_new_uri(
		world->lilv_world(), LILV_NS_RDF "type");
	LilvNode* prop = lilv_new_uri(
		world->lilv_world(), prop_uri.c_str());

	LilvNodes* ranges = lilv_world_find_nodes(
		world->lilv_world(), prop, rdfs_range, NULL);
	LILV_FOREACH(nodes, r, ranges) {
		const LilvNode* range = lilv_nodes_get(ranges, r);
		LilvNodes* objects = lilv_world_find_nodes(
			world->lilv_world(), NULL, rdf_type, range);
		LILV_FOREACH(nodes, o, objects) {
			const LilvNode* object = lilv_nodes_get(objects, o);
			const Glib::ustring label = get_label(world, object);
			if (!label.empty()) {
				Gtk::ListStore::iterator vi = _value_store->append();
				Gtk::ListStore::Row vrow = *vi;
				vrow[_combo_columns.uri_col]   = lilv_node_as_string(object);
				vrow[_combo_columns.label_col] = label;
			}
		}
	}

	lilv_node_free(prop);
	lilv_node_free(rdf_type);
	lilv_node_free(rdfs_range);
}

void
PropertiesWindow::add_clicked()
{
	if (!_key_combo->get_active() || !_value_combo->get_active()) {
		return;
	}

	const Gtk::ListStore::Row krow      = *(_key_combo->get_active());
	const Gtk::ListStore::Row vrow      = *(_value_combo->get_active());
	Glib::ustring             key_uri   = krow[_combo_columns.uri_col];
	Glib::ustring             value_uri = vrow[_combo_columns.uri_col];

	Raul::Atom value = _app->forge().alloc_uri(value_uri);

	Resource::Properties properties;
	properties.insert(make_pair(key_uri.c_str(), value));
	_app->interface()->put(_model->path(), properties);
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
	LOG(debug) << "apply {" << endl;
	Resource::Properties properties;
	for (Records::const_iterator r = _records.begin(); r != _records.end(); ++r) {
		const Raul::URI& uri    = r->first;
		const Record&    record = r->second;
		if (!_model->has_property(uri, record.value)) {
			LOG(debug) << "\t" << uri
			           << " = " << _app->forge().str(record.value) << endl;
			properties.insert(make_pair(uri, record.value));
		}
	}

	_app->interface()->put(_model->path(), properties);

	LOG(debug) << "}" << endl;
}

void
PropertiesWindow::ok_clicked()
{
	apply_clicked();
	cancel_clicked();
}

} // namespace GUI
} // namespace Ingen
