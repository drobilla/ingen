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

#ifndef INGEN_GUI_NEWSUBGRAPHWINDOW_HPP
#define INGEN_GUI_NEWSUBGRAPHWINDOW_HPP

#include "Window.hpp"

#include <ingen/Properties.hpp>

#include <memory>

namespace Glib {
template <class T> class RefPtr;
} // namespace Glib

namespace Gtk {
class Builder;
class Button;
class Entry;
class Label;
class SpinButton;
} // namespace Gtk

namespace ingen {

namespace client {
class GraphModel;
} // namespace client

namespace gui {

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

	void set_graph(std::shared_ptr<const client::GraphModel> graph);

	void present(std::shared_ptr<const client::GraphModel> graph,
	             const Properties&                         data);

private:
	void name_changed();
	void ok_clicked();
	void cancel_clicked();

	Properties                                _initial_data;
	std::shared_ptr<const client::GraphModel> _graph;

	Gtk::Entry*      _name_entry{nullptr};
	Gtk::Label*      _message_label{nullptr};
	Gtk::SpinButton* _poly_spinbutton{nullptr};
	Gtk::Button*     _ok_button{nullptr};
	Gtk::Button*     _cancel_button{nullptr};
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_NEWSUBGRAPHWINDOW_HPP
