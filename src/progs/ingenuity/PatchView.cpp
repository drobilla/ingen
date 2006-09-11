/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
 * 
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "PatchView.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include "App.h"
#include "ModelEngineInterface.h"
#include "OmFlowCanvas.h"
#include "PatchController.h"
#include "LoadPluginWindow.h"
#include "PatchModel.h"
#include "NewSubpatchWindow.h"
#include "LoadSubpatchWindow.h"
#include "NodeControlWindow.h"
#include "PatchPropertiesWindow.h"
#include "PatchTreeWindow.h"

namespace Ingenuity {


PatchView::PatchView(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::Box(cobject),
  _patch(NULL),
  _canvas(NULL),
  _breadcrumb_container(NULL),
  _enable_signal(true)
{
	property_visible() = false;

	xml->get_widget("patch_view_breadcrumb_container", _breadcrumb_container);
	xml->get_widget("patch_view_process_but", _process_but);
	xml->get_widget("patch_view_poly_spin", _poly_spin);
	xml->get_widget("patch_view_clear_but", _clear_but);
	xml->get_widget("patch_view_destroy_but", _destroy_but);
	xml->get_widget("patch_view_refresh_but", _refresh_but);
	xml->get_widget("patch_view_save_but", _save_but);
	xml->get_widget("patch_view_zoom_full_but", _zoom_full_but);
	xml->get_widget("patch_view_zoom_normal_but", _zoom_normal_but);
	xml->get_widget("patch_view_scrolledwindow", _canvas_scrolledwindow);
}


void
PatchView::patch_controller(PatchController* pc)
{
	_patch = pc;
	_canvas = new OmFlowCanvas(pc, 1600*2, 1200*2);

	_canvas_scrolledwindow->add(*_canvas);

	_poly_spin->set_value(pc->patch_model()->poly());
	_destroy_but->set_sensitive(pc->path() != "/");
	//_description_window->patch_model(pc->model());
	

	pc->patch_model()->enabled_sig.connect(sigc::mem_fun(this, &PatchView::enable));
	pc->patch_model()->disabled_sig.connect(sigc::mem_fun(this, &PatchView::disable));
	
	_process_but->signal_toggled().connect(sigc::mem_fun(this, &PatchView::process_toggled));
	
	_clear_but->signal_clicked().connect(sigc::mem_fun(this, &PatchView::clear_clicked));

	_zoom_normal_but->signal_clicked().connect(sigc::bind(sigc::mem_fun(
		static_cast<FlowCanvas*>(_canvas), &FlowCanvas::set_zoom), 1.0));

	_zoom_full_but->signal_clicked().connect(
		sigc::mem_fun(static_cast<FlowCanvas*>(_canvas), &FlowCanvas::zoom_full));
}


void
PatchView::show_control_window()
{
	if (_patch != NULL)
		_patch->show_control_window();
}


void
PatchView::process_toggled()
{
	if (!_enable_signal)
		return;

	if (_process_but->get_active()) {
		App::instance().engine()->enable_patch(_patch->model()->path());
		App::instance().patch_tree()->patch_enabled(_patch->model()->path());
	} else {
		App::instance().engine()->disable_patch(_patch->model()->path());
		App::instance().patch_tree()->patch_disabled(_patch->model()->path());
	}
}

void
PatchView::clear_clicked()
{
	App::instance().engine()->clear_patch(_patch->path());
}

void
PatchView::enable()
{
	_enable_signal = false;
	_process_but->set_active(true);
	_enable_signal = true;
}


void
PatchView::disable()
{
	_enable_signal = false;
	_process_but->set_active(false);
	_enable_signal = true;
}


} // namespace Ingenuity
