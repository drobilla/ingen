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

#include "ingen/client/ClientStore.hpp"
#include "ingen/client/GraphModel.hpp"

#include "App.hpp"
#include "GraphCanvas.hpp"
#include "GraphView.hpp"
#include "GraphWindow.hpp"
#include "WindowFactory.hpp"

namespace Ingen {
namespace GUI {

GraphWindow::GraphWindow(BaseObjectType*                   cobject,
                         const Glib::RefPtr<Gtk::Builder>& xml)
	: Window(cobject)
	, _box(NULL)
	, _position_stored(false)
	, _x(0)
	, _y(0)
{
	property_visible() = false;

	xml->get_widget_derived("graph_win_vbox", _box);

	set_title("Ingen");
}

GraphWindow::~GraphWindow()
{
	delete _box;
}

void
GraphWindow::init_window(App& app)
{
	Window::init_window(app);
	_box->init_box(app);
	_box->set_window(this);
}

void
GraphWindow::on_show()
{
	if (_position_stored)
		move(_x, _y);

	Gtk::Window::on_show();

	_box->view()->canvas()->widget().grab_focus();
}

void
GraphWindow::on_hide()
{
	_position_stored = true;
	get_position(_x, _y);
	Gtk::Window::on_hide();
}

bool
GraphWindow::on_key_press_event(GdkEventKey* event)
{
	// Disable Window C-w handling so quit works correctly
	return Gtk::Window::on_key_press_event(event);
}

} // namespace GUI
} // namespace Ingen
