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

#include "ingen/Node.hpp"
#include "ingen/memory.hpp"

#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/label.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/spinbutton.h>

namespace ingen {

namespace client { class GraphModel; }

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

	void set_graph(const SPtr<const client::GraphModel>& graph);

	void present(const SPtr<const client::GraphModel>& graph,
	             bool                                  import,
	             const Properties&                     data);

protected:
	void on_show() override;

private:
	void merge_ports_selected();
	void insert_ports_selected();

	void selection_changed();
	void cancel_clicked();
	void ok_clicked();

	Raul::Symbol symbol_from_filename(const Glib::ustring& filename);
	Raul::Symbol avoid_symbol_clash(const Raul::Symbol& symbol);

	App* _app = nullptr;

	Properties _initial_data;

	SPtr<const client::GraphModel> _graph;

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
