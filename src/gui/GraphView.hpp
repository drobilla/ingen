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

#include <gtkmm/box.h>

#include <memory>

namespace Glib {
template <class T> class RefPtr;
} // namespace Glib

namespace Gtk {
class Builder;
class ScrolledWindow;
class SpinButton;
class ToggleToolButton;
class ToolItem;
class Toolbar;
} // namespace Gtk

namespace ingen {

class Atom;
class URI;

namespace client {
class GraphModel;
} // namespace client

namespace gui {

class App;
class GraphCanvas;

/** The graph specific contents of a GraphWindow (ie the canvas and whatever else).
 *
 * \ingroup GUI
 */
class GraphView : public Gtk::Box
{
public:
	GraphView(BaseObjectType*                   cobject,
	          const Glib::RefPtr<Gtk::Builder>& xml);

	~GraphView() override;

	void init(App& app);

	std::shared_ptr<GraphCanvas>              canvas() const { return _canvas; }
	std::shared_ptr<const client::GraphModel> graph() const { return _graph; }

	Gtk::ToolItem* breadcrumb_container() const
	{
		return _breadcrumb_container;
	}

	static std::shared_ptr<GraphView>
	create(App& app, const std::shared_ptr<const client::GraphModel>& graph);

private:
	void set_graph(const std::shared_ptr<const client::GraphModel>& graph);

	void process_toggled();
	void poly_changed();
	void clear_clicked();

	void property_changed(const URI& predicate, const Atom& value);

	App* _app = nullptr;

	std::shared_ptr<const client::GraphModel> _graph;
	std::shared_ptr<GraphCanvas>              _canvas;

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
