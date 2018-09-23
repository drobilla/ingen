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

#ifndef INGEN_GUI_PROPERTIES_WINDOW_HPP
#define INGEN_GUI_PROPERTIES_WINDOW_HPP

#include <map>

#include <gtkmm/alignment.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/table.h>

#include "ingen/client/BlockModel.hpp"
#include "ingen/types.hpp"

#include "Window.hpp"

namespace ingen {

namespace client { class ObjectModel; }

namespace gui {

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

	void present(SPtr<const client::ObjectModel> model);
	void set_object(SPtr<const client::ObjectModel> model);

private:
	/** Record of a property (row in the table) */
	struct Record {
		Record(const Atom& v, Gtk::Alignment* vw, int r, Gtk::CheckButton* cb)
			: value(v), value_widget(vw), row(r), present_button(cb)
		{}
		Atom              value;
		Gtk::Alignment*   value_widget;
		int               row;
		Gtk::CheckButton* present_button;
	};

	struct ComboColumns : public Gtk::TreeModel::ColumnRecord {
		ComboColumns() {
			add(label_col);
			add(uri_col);
		}
		Gtk::TreeModelColumn<Glib::ustring> label_col;
		Gtk::TreeModelColumn<Glib::ustring> uri_col;
	};

	void add_property(const URI& key, const Atom& value);
	void change_property(const URI& key, const Atom& value);
	void remove_property(const URI& key, const Atom& value);
	void on_change(const URI& key);

	bool datatype_supported(const std::set<URI>& types,
	                        URI*                 widget_type);

	bool class_supported(const std::set<URI>& types);

	Gtk::Widget* create_value_widget(const URI&  key,
	                                 const char* type_uri,
	                                 const Atom& value = Atom());

	Atom get_value(LV2_URID type, Gtk::Widget* value_widget);

	void reset();
	void on_show();

	std::string active_key() const;

	void key_changed();
	void add_clicked();
	void cancel_clicked();
	void apply_clicked();
	void ok_clicked();

	typedef std::map<URI, Record> Records;
	Records _records;

	SPtr<const client::ObjectModel> _model;
	ComboColumns                    _combo_columns;
	Glib::RefPtr<Gtk::ListStore>    _key_store;
	sigc::connection                _property_connection;
	sigc::connection                _property_removed_connection;
	Gtk::VBox*                      _vbox;
	Gtk::ScrolledWindow*            _scrolledwindow;
	Gtk::Table*                     _table;
	Gtk::ComboBox*                  _key_combo;
	LV2_URID                        _value_type;
	Gtk::Bin*                       _value_bin;
	Gtk::Button*                    _add_button;
	Gtk::Button*                    _cancel_button;
	Gtk::Button*                    _apply_button;
	Gtk::Button*                    _ok_button;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_PROPERTIES_WINDOW_HPP
