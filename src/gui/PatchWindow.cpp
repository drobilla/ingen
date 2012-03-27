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

#include "ingen/client/ClientStore.hpp"
#include "ingen/client/PatchModel.hpp"

#include "App.hpp"
#include "PatchCanvas.hpp"
#include "PatchView.hpp"
#include "PatchWindow.hpp"
#include "WindowFactory.hpp"

namespace Ingen {
namespace GUI {

PatchWindow::PatchWindow(BaseObjectType*                   cobject,
                         const Glib::RefPtr<Gtk::Builder>& xml)
	: Window(cobject)
	, _box(NULL)
	, _position_stored(false)
	, _x(0)
	, _y(0)
{
	property_visible() = false;

	xml->get_widget_derived("patch_win_vbox", _box);

	//set_title(_patch->path().chop_scheme() + " - Ingen");
	set_title("Ingen");
}

PatchWindow::~PatchWindow()
{
	delete _box;
}

void
PatchWindow::init_window(App& app)
{
	Window::init_window(app);
	_box->init_box(app);
	_box->set_window(this);
}

void
PatchWindow::set_patch_from_path(const Raul::Path& path, SharedPtr<PatchView> view)
{
	if (view) {
		assert(view->patch()->path() == path);
		_app->window_factory()->present_patch(view->patch(), this, view);
	} else {
		SharedPtr<const PatchModel> model = PtrCast<const PatchModel>(
			_app->store()->object(path));
		if (model)
			_app->window_factory()->present_patch(model, this);
	}
}

void
PatchWindow::on_show()
{
	if (_position_stored)
		move(_x, _y);

	Gtk::Window::on_show();
}

void
PatchWindow::on_hide()
{
	_position_stored = true;
	get_position(_x, _y);
	Gtk::Window::on_hide();
}

bool
PatchWindow::on_event(GdkEvent* event)
{
	if ((event->type == GDK_KEY_PRESS || event->type == GDK_KEY_RELEASE)
	    && box()->view()->canvas()->on_event(event)) {
		return true;
	} else {
		return Gtk::Window::on_event(event);
	}
}

} // namespace GUI
} // namespace Ingen
