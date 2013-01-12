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

using namespace std;

namespace Ingen {

using namespace Client;

namespace GUI {

GraphView::GraphView(BaseObjectType*                   cobject,
                     const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::Box(cobject)
	, _app(NULL)
	, _breadcrumb_container(NULL)
	, _enable_signal(true)
{
	property_visible() = false;

	xml->get_widget("graph_view_breadcrumb_container", _breadcrumb_container);
	xml->get_widget("graph_view_toolbar", _toolbar);
	xml->get_widget("graph_view_process_but", _process_but);
	xml->get_widget("graph_view_poly_spin", _poly_spin);
	xml->get_widget("graph_view_refresh_but", _refresh_but);
	xml->get_widget("graph_view_save_but", _save_but);
	xml->get_widget("graph_view_zoom_full_but", _zoom_full_but);
	xml->get_widget("graph_view_zoom_normal_but", _zoom_normal_but);
	xml->get_widget("graph_view_scrolledwindow", _canvas_scrolledwindow);

	_toolbar->set_toolbar_style(Gtk::TOOLBAR_ICONS);
	_canvas_scrolledwindow->property_hadjustment().get_value()->set_step_increment(10);
	_canvas_scrolledwindow->property_vadjustment().get_value()->set_step_increment(10);
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

	for (const auto& p : graph->properties())
		property_changed(p.first, p.second);

	// Connect model signals to track state
	graph->signal_property().connect(
		sigc::mem_fun(this, &GraphView::property_changed));

	// Connect widget signals to do things
	_process_but->signal_toggled().connect(
		sigc::mem_fun(this, &GraphView::process_toggled));
	_refresh_but->signal_clicked().connect(
		sigc::mem_fun(this, &GraphView::refresh_clicked));

	_zoom_normal_but->signal_clicked().connect(sigc::bind(sigc::mem_fun(
		_canvas.get(), &GraphCanvas::set_zoom), 1.0));

	_zoom_full_but->signal_clicked().connect(
		sigc::mem_fun(_canvas.get(), &GraphCanvas::zoom_full));

	_poly_spin->signal_value_changed().connect(
			sigc::mem_fun(*this, &GraphView::poly_changed));

	#if 0
	_canvas->signal_item_entered.connect(
			sigc::mem_fun(*this, &GraphView::canvas_item_entered));

	_canvas->signal_item_left.connect(
			sigc::mem_fun(*this, &GraphView::canvas_item_left));
	#endif

	_canvas->widget().grab_focus();
}

SPtr<GraphView>
GraphView::create(App& app, SPtr<const GraphModel> graph)
{
	GraphView* result = NULL;
	Glib::RefPtr<Gtk::Builder> xml = WidgetFactory::create("warehouse_win");
	xml->get_widget_derived("graph_view_box", result);
	result->init(app);
	result->set_graph(graph);
	return SPtr<GraphView>(result);
}

#if 0
void
GraphView::canvas_item_entered(Gnome::Canvas::Item* item)
{
	NodeModule* m = dynamic_cast<NodeModule*>(item);
	if (m)
		signal_object_entered.emit(m->block().get());

	const Port* p = dynamic_cast<const Port*>(item);
	if (p)
		signal_object_entered.emit(p->model().get());
}

void
GraphView::canvas_item_left(Gnome::Canvas::Item* item)
{
	NodeModule* m = dynamic_cast<NodeModule*>(item);
	if (m) {
		signal_object_left.emit(m->block().get());
		return;
	}

	const Port* p = dynamic_cast<const Port*>(item);
	if (p)
		signal_object_left.emit(p->model().get());
}
#endif

void
GraphView::process_toggled()
{
	if (!_enable_signal)
		return;

	_app->interface()->set_property(
		_graph->uri(),
		_app->uris().ingen_enabled,
		_app->forge().make((bool)_process_but->get_active()));
}

void
GraphView::poly_changed()
{
	const int poly = _poly_spin->get_value_as_int();
	if (_enable_signal && poly != (int)_graph->internal_poly()) {
		_app->interface()->set_property(
			_graph->uri(),
			_app->uris().ingen_polyphony,
			_app->forge().make(poly));
	}
}

void
GraphView::refresh_clicked()
{
	_app->interface()->get(_graph->uri());
}

void
GraphView::property_changed(const Raul::URI& predicate, const Raul::Atom& value)
{
	_enable_signal = false;
	if (predicate == _app->uris().ingen_enabled) {
		if (value.type() == _app->uris().forge.Bool) {
			_process_but->set_active(value.get_bool());
		}
	} else if (predicate == _app->uris().ingen_polyphony) {
		if (value.type() == _app->uris().forge.Int) {
			_poly_spin->set_value(value.get_int32());
		}
	}
	_enable_signal = true;
}

} // namespace GUI
} // namespace Ingen
