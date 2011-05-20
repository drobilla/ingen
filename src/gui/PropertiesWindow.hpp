/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef INGEN_GUI_PROPERTIES_WINDOW_HPP
#define INGEN_GUI_PROPERTIES_WINDOW_HPP

#include <gtkmm.h>

#include "raul/SharedPtr.hpp"

#include "client/NodeModel.hpp"

#include "Window.hpp"

using namespace Ingen::Client;

namespace Ingen {
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

	void present(SharedPtr<const ObjectModel> model);
	void set_object(SharedPtr<const ObjectModel> model);

private:
	/** Record of a property (row in the table) */
	struct Record {
		Record(const Raul::Atom& v, Gtk::ComboBox* tw, Gtk::Alignment* vw, int r)
			: value(v), type_widget(tw), value_widget(vw), row(r)
		{}
		Raul::Atom      value;
		Gtk::ComboBox*  type_widget;
		Gtk::Alignment* value_widget;
		int             row;
	};

	/** Columns for type combo in treeview */
	class TypeColumns : public Gtk::TreeModel::ColumnRecord {
	public:
		TypeColumns() { add(type); add(choice); }

		Gtk::TreeModelColumn<Raul::Atom::Type> type;
		Gtk::TreeModelColumn<Glib::ustring>    choice;
	};

	Gtk::Widget* create_value_widget(const Raul::URI& uri, const Raul::Atom& value);

	void reset();
	void on_show();

	void property_changed(const Raul::URI& predicate, const Raul::Atom& value);

	void value_edited(const Raul::URI& predicate);

	void cancel_clicked();
	void apply_clicked();
	void ok_clicked();

	typedef std::map<Raul::URI, Record> Records;
	Records _records;

	TypeColumns                  _type_cols;
	Glib::RefPtr<Gtk::ListStore> _type_choices;

	SharedPtr<const ObjectModel> _model;
	sigc::connection             _property_connection;
	Gtk::VBox*                   _vbox;
	Gtk::ScrolledWindow*         _scrolledwindow;
	Gtk::Table*                  _table;
	Gtk::Button*                 _cancel_button;
	Gtk::Button*                 _apply_button;
	Gtk::Button*                 _ok_button;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_PROPERTIES_WINDOW_HPP
