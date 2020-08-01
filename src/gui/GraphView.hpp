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

#ifndef INGEN_GUI_GRAPHVIEW_HPP
#define INGEN_GUI_GRAPHVIEW_HPP

#include "ingen/types.hpp"

#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/toolitem.h>
#include <gtkmm/toolitem.h>

namespace Raul { class Atom; }

namespace ingen {

class Atom;
class URI;

namespace client {
class PortModel;
class MetadataModel;
class GraphModel;
class ObjectModel;
}

namespace gui {

class App;
class LoadPluginWindow;
class NewSubgraphWindow;
class GraphCanvas;
class GraphDescriptionWindow;
class SubgraphModule;

/** The graph specific contents of a GraphWindow (ie the canvas and whatever else).
 *
 * \ingroup GUI
 */
class GraphView : public Gtk::Box
{
public:
	GraphView(BaseObjectType*                   cobject,
	          const Glib::RefPtr<Gtk::Builder>& xml);

	~GraphView();

	void init(App& app);

	SPtr<GraphCanvas>              canvas()               const { return _canvas; }
	SPtr<const client::GraphModel> graph()                const { return _graph; }
	Gtk::ToolItem*                 breadcrumb_container() const { return _breadcrumb_container; }

	static SPtr<GraphView>
	create(App& app, const SPtr<const client::GraphModel>& graph);

private:
	void set_graph(const SPtr<const client::GraphModel>& graph);

	void process_toggled();
	void poly_changed();
	void clear_clicked();

	void property_changed(const URI& predicate, const Atom& value);

	App* _app = nullptr;

	SPtr<const client::GraphModel> _graph;
	SPtr<GraphCanvas>              _canvas;

	Gtk::ScrolledWindow*   _canvas_scrolledwindow = nullptr;
	Gtk::Toolbar*          _toolbar = nullptr;
	Gtk::ToggleToolButton* _process_but = nullptr;
	Gtk::SpinButton*       _poly_spin = nullptr;
	Gtk::ToolItem*         _breadcrumb_container = nullptr;

	bool _enable_signal = true;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_GRAPHVIEW_HPP
