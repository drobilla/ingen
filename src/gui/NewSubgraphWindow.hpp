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

#ifndef INGEN_GUI_NEWSUBGRAPHWINDOW_HPP
#define INGEN_GUI_NEWSUBGRAPHWINDOW_HPP

#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/spinbutton.h>

#include "raul/SharedPtr.hpp"

#include "ingen/Node.hpp"

#include "Window.hpp"

namespace Ingen {

namespace Client { class GraphModel; }

namespace GUI {

/** 'New Subgraph' window.
 *
 * Loaded from XML as a derived object.
 *
 * \ingroup GUI
 */
class NewSubgraphWindow : public Window
{
public:
	NewSubgraphWindow(BaseObjectType*                   cobject,
	                  const Glib::RefPtr<Gtk::Builder>& xml);

	void set_graph(SharedPtr<const Client::GraphModel> graph);

	void present(SharedPtr<const Client::GraphModel> graph,
	             Node::Properties                    data);

private:
	void name_changed();
	void ok_clicked();
	void cancel_clicked();

	Node::Properties                    _initial_data;
	SharedPtr<const Client::GraphModel> _graph;

	Gtk::Entry*      _name_entry;
	Gtk::Label*      _message_label;
	Gtk::SpinButton* _poly_spinbutton;
	Gtk::Button*     _ok_button;
	Gtk::Button*     _cancel_button;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_NEWSUBGRAPHWINDOW_HPP
