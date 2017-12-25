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

#include <cassert>
#include <fstream>

#include "ingen/Interface.hpp"
#include "ingen/client/GraphModel.hpp"

#include "App.hpp"
#include "LoadPluginWindow.hpp"
#include "NewSubgraphWindow.hpp"
#include "GraphCanvas.hpp"
#include "GraphTreeWindow.hpp"
#include "GraphView.hpp"
#include "WidgetFactory.hpp"

namespace Ingen {

using namespace Client;

namespace GUI {

GraphView::GraphView(BaseObjectType*                   cobject,
                     const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::Box(cobject)
	, _app(nullptr)
	, _breadcrumb_container(nullptr)
	, _enable_signal(true)
{
	property_visible() = false;

	xml->get_widget("graph_view_breadcrumb_container", _breadcrumb_container);
	xml->get_widget("graph_view_toolbar", _toolbar);
	xml->get_widget("graph_view_process_but", _process_but);
	xml->get_widget("graph_view_poly_spin", _poly_spin);
	xml->get_widget("graph_view_scrolledwindow", _canvas_scrolledwindow);

	_toolbar->set_toolbar_style(Gtk::TOOLBAR_ICONS);
	_canvas_scrolledwindow->property_hadjustment().get_value()->set_step_increment(10);
	_canvas_scrolledwindow->property_vadjustment().get_value()->set_step_increment(10);
}

GraphView::~GraphView()
{
	_canvas_scrolledwindow->remove();
}

void
GraphView::init(App& app)
{
	_app = &app;
}

void
GraphView::set_graph(SPtr<const GraphModel> graph)
{
	assert(!_canvas); // FIXME: remove

	assert(_breadcrumb_container); // ensure created

	_graph = graph;
	_canvas = SPtr<GraphCanvas>(new GraphCanvas(*_app, graph, 1600*2, 1200*2));
	_canvas->build();

	_canvas_scrolledwindow->add(_canvas->widget());

	_poly_spin->set_range(1, 128);
	_poly_spin->set_increments(1, 4);
	_poly_spin->set_value(graph->internal_poly());

	for (const auto& p : graph->properties()) {
		property_changed(p.first, p.second);
	}

	// Connect model signals to track state
	graph->signal_property().connect(
		sigc::mem_fun(this, &GraphView::property_changed));

	// Connect widget signals to do things
	_process_but->signal_toggled().connect(
		sigc::mem_fun(this, &GraphView::process_toggled));

	_poly_spin->signal_value_changed().connect(
		sigc::mem_fun(*this, &GraphView::poly_changed));

	_canvas->widget().grab_focus();
}

SPtr<GraphView>
GraphView::create(App& app, SPtr<const GraphModel> graph)
{
	GraphView* result = nullptr;
	Glib::RefPtr<Gtk::Builder> xml = WidgetFactory::create("warehouse_win");
	xml->get_widget_derived("graph_view_box", result);
	result->init(app);
	result->set_graph(graph);
	return SPtr<GraphView>(result);
}

void
GraphView::process_toggled()
{
	if (!_enable_signal) {
		return;
	}

	_app->set_property(_graph->uri(),
	                   _app->uris().ingen_enabled,
	                   _app->forge().make((bool)_process_but->get_active()));
}

void
GraphView::poly_changed()
{
	const int poly = _poly_spin->get_value_as_int();
	if (_enable_signal && poly != (int)_graph->internal_poly()) {
		_app->set_property(_graph->uri(),
		                   _app->uris().ingen_polyphony,
		                   _app->forge().make(poly));
	}
}

void
GraphView::property_changed(const Raul::URI& predicate, const Atom& value)
{
	_enable_signal = false;
	if (predicate == _app->uris().ingen_enabled) {
		if (value.type() == _app->uris().forge.Bool) {
			_process_but->set_active(value.get<int32_t>());
		}
	} else if (predicate == _app->uris().ingen_polyphony) {
		if (value.type() == _app->uris().forge.Int) {
			_poly_spin->set_value(value.get<int32_t>());
		}
	}
	_enable_signal = true;
}

} // namespace GUI
} // namespace Ingen
