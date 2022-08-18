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

#include "Window.hpp"

#include "ingen/Atom.hpp"
#include "ingen/URI.hpp"
#include "lv2/urid/urid.h"

#include <glibmm/refptr.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treemodelcolumn.h>
#include <gtkmm/window.h>
#include <sigc++/connection.h>

#include <map>
#include <memory>
#include <set>
#include <string>

namespace Glib {
class ustring;
} // namespace Glib

namespace Gtk {
class Alignment;
class Bin;
class Builder;
class Button;
class CheckButton;
class ComboBox;
class ScrolledWindow;
class Table;
class VBox;
class Widget;
} // namespace Gtk

namespace ingen {

namespace client {
class ObjectModel;
} // namespace client

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

	void present(const std::shared_ptr<const client::ObjectModel>& model);
	void set_object(const std::shared_ptr<const client::ObjectModel>& model);

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
	void on_show() override;

	std::string active_key() const;

	void key_changed();
	void add_clicked();
	void cancel_clicked();
	void apply_clicked();
	void ok_clicked();

	using Records = std::map<URI, Record>;
	Records _records;

	std::shared_ptr<const client::ObjectModel> _model;
	ComboColumns                               _combo_columns;
	Glib::RefPtr<Gtk::ListStore>               _key_store;
	sigc::connection                           _property_connection;
	sigc::connection                           _property_removed_connection;
	Gtk::VBox*                                 _vbox{nullptr};
	Gtk::ScrolledWindow*                       _scrolledwindow{nullptr};
	Gtk::Table*                                _table{nullptr};
	Gtk::ComboBox*                             _key_combo{nullptr};
	LV2_URID                                   _value_type{0};
	Gtk::Bin*                                  _value_bin{nullptr};
	Gtk::Button*                               _add_button{nullptr};
	Gtk::Button*                               _cancel_button{nullptr};
	Gtk::Button*                               _apply_button{nullptr};
	Gtk::Button*                               _ok_button{nullptr};
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_PROPERTIES_WINDOW_HPP
