/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include <iostream>
#include <cassert>
#include <fstream>
#include "interface/EngineInterface.h"
#include "client/PatchModel.h"
#include "App.h"
#include "PatchView.h"
#include "PatchCanvas.h"
#include "LoadPluginWindow.h"
#include "NewSubpatchWindow.h"
#include "LoadSubpatchWindow.h"
#include "NodeControlWindow.h"
#include "PatchPropertiesWindow.h"
#include "PatchTreeWindow.h"
#include "GladeFactory.h"

namespace Ingenuity {


PatchView::PatchView(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::Box(cobject),
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
PatchView::set_patch(SharedPtr<PatchModel> patch)
{
	assert(!_canvas); // FIXME: remove

	cerr << "Creating view for " << patch->path() << endl;

	assert(_breadcrumb_container); // ensure created

	_patch = patch;
	_canvas = SharedPtr<PatchCanvas>(new PatchCanvas(patch, 1600*2, 1200*2));
	_canvas->build();

	_canvas_scrolledwindow->add(*_canvas);

	_poly_spin->set_value(patch->poly());
	_destroy_but->set_sensitive(patch->path() != "/");
	patch->enabled() ? enable() : disable();

	// Connect model signals to track state
	patch->enabled_sig.connect(sigc::mem_fun(this, &PatchView::enable));
	patch->disabled_sig.connect(sigc::mem_fun(this, &PatchView::disable));

	// Connect widget signals to do things
	_process_but->signal_toggled().connect(sigc::mem_fun(this, &PatchView::process_toggled));
	_clear_but->signal_clicked().connect(sigc::mem_fun(this, &PatchView::clear_clicked));
	_refresh_but->signal_clicked().connect(sigc::mem_fun(this, &PatchView::refresh_clicked));

	_zoom_normal_but->signal_clicked().connect(sigc::bind(sigc::mem_fun(
		_canvas.get(), &FlowCanvas::set_zoom), 1.0));

	_zoom_full_but->signal_clicked().connect(
		sigc::mem_fun(_canvas.get(), &FlowCanvas::zoom_full));
}


PatchView::~PatchView()
{
	cerr << "Destroying view for " << _patch->path() << endl;
}


SharedPtr<PatchView>
PatchView::create(SharedPtr<PatchModel> patch)

{
	const Glib::RefPtr<Gnome::Glade::Xml>& xml = GladeFactory::new_glade_reference("patch_view_box");
	PatchView* result = NULL;
	xml->get_widget_derived("patch_view_box", result);
	assert(result);
	result->set_patch(patch);
	return SharedPtr<PatchView>(result);
}


void
PatchView::process_toggled()
{
	if (!_enable_signal)
		return;

	if (_process_but->get_active()) {
		App::instance().engine()->enable_patch(_patch->path());
		App::instance().patch_tree()->patch_enabled(_patch->path());
	} else {
		App::instance().engine()->disable_patch(_patch->path());
		App::instance().patch_tree()->patch_disabled(_patch->path());
	}
}


void
PatchView::clear_clicked()
{
	App::instance().engine()->clear_patch(_patch->path());
}


void
PatchView::refresh_clicked()
{
	App::instance().engine()->request_object(_patch->path());
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
