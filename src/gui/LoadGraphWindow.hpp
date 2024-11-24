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

#ifndef INGEN_GUI_LOADGRAPHWINDOW_HPP
#define INGEN_GUI_LOADGRAPHWINDOW_HPP

#include <ingen/Properties.hpp>
#include <raul/Symbol.hpp>

#include <glibmm/ustring.h>
#include <gtkmm/filechooserdialog.h>

#include <memory>

namespace Glib {
template <class T> class RefPtr;
} // namespace Glib

namespace Gtk {
class Builder;
class Button;
class Entry;
class Label;
class RadioButton;
class SpinButton;
} // namespace Gtk

namespace ingen {

namespace client {
class GraphModel;
} // namespace client

namespace gui {

class App;

/** 'Load Graph' Window.
 *
 * Loaded from XML as a derived object.
 *
 * \ingroup GUI
 */
class LoadGraphWindow : public Gtk::FileChooserDialog
{
public:
	LoadGraphWindow(BaseObjectType*                   cobject,
	                const Glib::RefPtr<Gtk::Builder>& xml);

	void init(App& app) { _app = &app; }

	void set_graph(const std::shared_ptr<const client::GraphModel>& graph);

	void present(const std::shared_ptr<const client::GraphModel>& graph,
	             bool                                             import,
	             const Properties&                                data);

protected:
	void on_show() override;

private:
	void merge_ports_selected();
	void insert_ports_selected();

	void selection_changed();
	void cancel_clicked();
	void ok_clicked();

	raul::Symbol symbol_from_filename(const Glib::ustring& filename);
	raul::Symbol avoid_symbol_clash(const raul::Symbol& symbol);

	App* _app = nullptr;

	Properties _initial_data;

	std::shared_ptr<const client::GraphModel> _graph;

	Gtk::Label*       _symbol_label = nullptr;
	Gtk::Entry*       _symbol_entry = nullptr;
	Gtk::Label*       _ports_label = nullptr;
	Gtk::RadioButton* _merge_ports_radio = nullptr;
	Gtk::RadioButton* _insert_ports_radio = nullptr;
	Gtk::RadioButton* _poly_voices_radio = nullptr;
	Gtk::RadioButton* _poly_from_file_radio = nullptr;
	Gtk::SpinButton*  _poly_spinbutton = nullptr;
	Gtk::Button*      _ok_button = nullptr;
	Gtk::Button*      _cancel_button = nullptr;

	bool _import = false;
	bool _merge_ports = false;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_LOADGRAPHWINDOW_HPP
