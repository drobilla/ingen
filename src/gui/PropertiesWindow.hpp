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

#ifndef INGEN_GUI_PROPERTIES_WINDOW_HPP
#define INGEN_GUI_PROPERTIES_WINDOW_HPP

#include <map>

#include <gtkmm/alignment.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/liststore.h>
#include <gtkmm/combobox.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/table.h>

#include "raul/SharedPtr.hpp"

#include "ingen/client/NodeModel.hpp"

#include "Window.hpp"

namespace Ingen {

namespace Client { class ObjectModel; }

namespace GUI {

/** Object properties window.
 *
 * Loaded from XML as a derived object.
 *
 * \ingroup GUI
 */
class PropertiesWindow : public Window
{
public:
	PropertiesWindow(BaseObjectType*                   cobject,
	                 const Glib::RefPtr<Gtk::Builder>& xml);

	void present(SharedPtr<const Client::ObjectModel> model);
	void set_object(SharedPtr<const Client::ObjectModel> model);

private:
	/** Record of a property (row in the table) */
	struct Record {
		Record(const Raul::Atom& v, Gtk::Alignment* vw, int r)
			: value(v), value_widget(vw), row(r)
		{}
		Raul::Atom      value;
		Gtk::Alignment* value_widget;
		int             row;
	};

	struct ComboColumns : public Gtk::TreeModel::ColumnRecord {
		ComboColumns() {
			add(label_col);
			add(uri_col);
		}
		Gtk::TreeModelColumn<Glib::ustring> label_col;
		Gtk::TreeModelColumn<Glib::ustring> uri_col;
	};

	void add_property(const Raul::URI&  uri,
	                  const Raul::Atom& value);

	Gtk::Widget* create_value_widget(const Raul::URI&  uri,
	                                 const Raul::Atom& value);

	void init();
	void reset();
	void on_show();

	void property_changed(const Raul::URI& predicate, const Raul::Atom& value);
	void value_edited(const Raul::URI& predicate);
	void key_changed();
	void add_clicked();
	void cancel_clicked();
	void apply_clicked();
	void ok_clicked();

	typedef std::map<Raul::URI, Record> Records;
	Records _records;

	SharedPtr<const Client::ObjectModel> _model;
	ComboColumns                         _combo_columns;
	Glib::RefPtr<Gtk::ListStore>         _key_store;
	Glib::RefPtr<Gtk::ListStore>         _value_store;
	sigc::connection                     _property_connection;
	Gtk::VBox*                           _vbox;
	Gtk::ScrolledWindow*                 _scrolledwindow;
	Gtk::Table*                          _table;
	Gtk::ComboBox*                       _key_combo;
	Gtk::ComboBox*                       _value_combo;
	Gtk::Button*                         _add_button;
	Gtk::Button*                         _cancel_button;
	Gtk::Button*                         _apply_button;
	Gtk::Button*                         _ok_button;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_PROPERTIES_WINDOW_HPP
