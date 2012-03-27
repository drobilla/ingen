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

#include <cassert>
#include <string>
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
namespace GUI {

PropertiesWindow::PropertiesWindow(BaseObjectType*                   cobject,
                                   const Glib::RefPtr<Gtk::Builder>& xml)
	: Window(cobject)
	, _initialised(false)
{
	xml->get_widget("properties_vbox", _vbox);
	xml->get_widget("properties_scrolledwindow", _scrolledwindow);
	xml->get_widget("properties_table", _table);
	xml->get_widget("properties_cancel_button", _cancel_button);
	xml->get_widget("properties_apply_button", _apply_button);
	xml->get_widget("properties_ok_button", _ok_button);

	_cancel_button->signal_clicked().connect(
			sigc::mem_fun(this, &PropertiesWindow::cancel_clicked));

	_apply_button->signal_clicked().connect(
			sigc::mem_fun(this, &PropertiesWindow::apply_clicked));

	_ok_button->signal_clicked().connect(
			sigc::mem_fun(this, &PropertiesWindow::ok_clicked));
}

void
PropertiesWindow::init()
{
	Forge& forge = _app->forge();
	Gtk::TreeModel::Row row = *_type_choices->append();
	row[_type_cols.type]   = forge.Int;
	row[_type_cols.choice] = "Int";

	row = *_type_choices->append();
	row[_type_cols.type]   = forge.Float;
	row[_type_cols.choice] = "Float";

	row = *_type_choices->append();
	row[_type_cols.type]   = forge.Bool;
	row[_type_cols.choice] = "Bool";

	row = *_type_choices->append();
	row[_type_cols.type]   = forge.URI;
	row[_type_cols.choice] = "URI";

	row = *_type_choices->append();
	row[_type_cols.type]   = forge.String;
	row[_type_cols.choice] = "String";

	_initialised = true;
}

void
PropertiesWindow::reset()
{
	_table->children().clear();
	_table->resize(1, 3);
	_table->property_n_rows() = 1;

	_records.clear();

	_property_connection.disconnect();
	_model.reset();
}

void
PropertiesWindow::present(SharedPtr<const ObjectModel> model)
{
	set_object(model);
	Gtk::Window::present();
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

	ostringstream ss;
	unsigned n_rows = 0;
	for (ObjectModel::Properties::const_iterator i = model->properties().begin();
			i != model->properties().end(); ++i) {
		_table->property_n_rows() = ++n_rows;

		const Raul::Atom& value = i->second;

		// Column 0: Property
		Gtk::Label* lab = manage(
			new Gtk::Label(world->rdf_world()->prefixes().qualify(i->first.str()),
			               0.0, 0.5));
		_table->attach(*lab, 0, 1, n_rows, n_rows + 1, Gtk::FILL|Gtk::SHRINK, Gtk::SHRINK);

		// Column 1: Type
		Gtk::ComboBox* combo = manage(new Gtk::ComboBox());
		combo->set_model(_type_choices);
		combo->pack_start(_type_cols.choice);
		const char path[] = { static_cast<int>(value.type()) - 1 + '0', '\0' };
		combo->set_active(_type_choices->get_iter(path));
		Gtk::Alignment* align = manage(new Gtk::Alignment(0.0, 0.5, 0.0, 1.0));
		align->add(*combo);
		_table->attach(*align, 1, 2, n_rows, n_rows + 1, Gtk::FILL|Gtk::SHRINK, Gtk::SHRINK);

		// Column 2: Value
		align = manage(new Gtk::Alignment(0.0, 0.5, 1.0, 0.0));
		Gtk::Widget* value_widget = create_value_widget(i->first, value);
		if (value_widget)
			align->add(*value_widget);
		_table->attach(*align, 2, 3, n_rows, n_rows + 1, Gtk::FILL|Gtk::EXPAND, Gtk::SHRINK);
		_records.insert(make_pair(i->first, Record(value, combo, align, n_rows)));
	}

	_table->show_all();

	_property_connection = model->signal_property().connect(
			sigc::mem_fun(this, &PropertiesWindow::property_changed));
}

Gtk::Widget*
PropertiesWindow::create_value_widget(const Raul::URI& uri, const Raul::Atom& value)
{
	Ingen::Forge& forge = _app->forge();
	if (value.type() == forge.Int) {
		Gtk::SpinButton* widget = manage(new Gtk::SpinButton(0.0, 0));
		widget->property_numeric() = true;
		widget->set_value(value.get_int32());
		widget->set_snap_to_ticks(true);
		widget->set_range(INT_MIN, INT_MAX);
		widget->set_increments(1, 10);
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
	static const int WIN_PAD  = 32;
	static const int VBOX_PAD = 12;
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
		LOG(error) << "Unknown property `" << predicate << "' changed" << endl;
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
	Raul::Atom::TypeID type   = (*record.type_widget->get_active())[_type_cols.type];
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
		record.value = _app->forge().alloc_uri(widget->get_text());
	}

	return;

bad_type:
	LOG(error) << "Property `" << predicate << "' value widget has wrong type" << endl;
	return;
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

	_app->engine()->put(_model->path(), properties);

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
