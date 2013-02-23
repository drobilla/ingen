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

#ifndef INGEN_GUI_GRAPHVIEW_HPP
#define INGEN_GUI_GRAPHVIEW_HPP

#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/toolitem.h>
#include <gtkmm/toolitem.h>

#include "ingen/types.hpp"
#include "raul/URI.hpp"

namespace Raul { class Atom; }

namespace Ingen {

namespace Client {
	class PortModel;
	class MetadataModel;
	class GraphModel;
	class ObjectModel;
}

namespace GUI {

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

	void init(App& app);

	SPtr<GraphCanvas>              canvas()               const { return _canvas; }
	SPtr<const Client::GraphModel> graph()                const { return _graph; }
	Gtk::ToolItem*                 breadcrumb_container() const { return _breadcrumb_container; }

	static SPtr<GraphView> create(App& app,
	                              SPtr<const Client::GraphModel> graph);

private:
	void set_graph(SPtr<const Client::GraphModel> graph);

	void process_toggled();
	void poly_changed();
	void clear_clicked();
	void refresh_clicked();

	void property_changed(const Raul::URI& predicate, const Atom& value);

	void zoom_full();

	App* _app;

	SPtr<const Client::GraphModel> _graph;
	SPtr<GraphCanvas>              _canvas;

	Gtk::ScrolledWindow*   _canvas_scrolledwindow;
	Gtk::Toolbar*          _toolbar;
	Gtk::ToggleToolButton* _process_but;
	Gtk::SpinButton*       _poly_spin;
	Gtk::ToolButton*       _refresh_but;
	Gtk::ToolButton*       _save_but;
	Gtk::ToolButton*       _zoom_normal_but;
	Gtk::ToolButton*       _zoom_full_but;
	Gtk::ToolItem*         _breadcrumb_container;

	bool _enable_signal;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_GRAPHVIEW_HPP
