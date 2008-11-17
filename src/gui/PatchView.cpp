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
#include "interface/EngineInterface.hpp"
#include "client/PatchModel.hpp"
#include "App.hpp"
#include "PatchView.hpp"
#include "PatchCanvas.hpp"
#include "LoadPluginWindow.hpp"
#include "NewSubpatchWindow.hpp"
#include "LoadSubpatchWindow.hpp"
#include "NodeControlWindow.hpp"
#include "PatchPropertiesWindow.hpp"
#include "PatchTreeWindow.hpp"
#include "GladeFactory.hpp"

namespace Ingen {
namespace GUI {


PatchView::PatchView(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::Box(cobject),
  _breadcrumb_container(NULL),
  _enable_signal(true)
{
	property_visible() = false;

	xml->get_widget("patch_view_breadcrumb_container", _breadcrumb_container);
	xml->get_widget("patch_view_toolbar", _toolbar);
	xml->get_widget("patch_view_process_but", _process_but);
	xml->get_widget("patch_view_poly_spin", _poly_spin);
	xml->get_widget("patch_view_clear_but", _clear_but);
	xml->get_widget("patch_view_destroy_but", _destroy_but);
	xml->get_widget("patch_view_refresh_but", _refresh_but);
	xml->get_widget("patch_view_save_but", _save_but);
	xml->get_widget("patch_view_zoom_full_but", _zoom_full_but);
	xml->get_widget("patch_view_zoom_normal_but", _zoom_normal_but);
	xml->get_widget("patch_view_edit_mode_but", _edit_mode_but);
	xml->get_widget("patch_view_scrolledwindow", _canvas_scrolledwindow);

	_toolbar->set_toolbar_style(Gtk::TOOLBAR_ICONS);
	_canvas_scrolledwindow->property_hadjustment().get_value()->set_step_increment(10);
	_canvas_scrolledwindow->property_vadjustment().get_value()->set_step_increment(10);

}


void
PatchView::set_patch(SharedPtr<PatchModel> patch)
{
	assert(!_canvas); // FIXME: remove

	//cerr << "Creating view for " << patch->path() << endl;

	assert(_breadcrumb_container); // ensure created

	_patch = patch;
	_canvas = SharedPtr<PatchCanvas>(new PatchCanvas(patch, 1600*2, 1200*2));
	_canvas->build();

	_canvas_scrolledwindow->add(*_canvas);

	_poly_spin->set_value(patch->poly());
	_destroy_but->set_sensitive(patch->path() != "/");

	for (GraphObject::Properties::const_iterator i = patch->properties().begin();
			i != patch->properties().end(); ++i)
		property_changed(i->first, i->second);

	// Connect model signals to track state
	patch->signal_property.connect(sigc::mem_fun(this, &PatchView::property_changed));

	// Connect widget signals to do things
	_process_but->signal_toggled().connect(sigc::mem_fun(this, &PatchView::process_toggled));
	_clear_but->signal_clicked().connect(sigc::mem_fun(this, &PatchView::clear_clicked));
	_refresh_but->signal_clicked().connect(sigc::mem_fun(this, &PatchView::refresh_clicked));

	_zoom_normal_but->signal_clicked().connect(sigc::bind(sigc::mem_fun(
		_canvas.get(), &PatchCanvas::set_zoom), 1.0));

	_zoom_full_but->signal_clicked().connect(
		sigc::mem_fun(_canvas.get(), &PatchCanvas::zoom_full));

	patch->signal_editable.connect(sigc::mem_fun(
			*this, &PatchView::on_editable_sig));

	_edit_mode_but->signal_toggled().connect(sigc::mem_fun(
			*this, &PatchView::editable_toggled));
	
	_poly_spin->signal_value_changed().connect(
			sigc::mem_fun(*this, &PatchView::poly_changed));

	_canvas->grab_focus();
}


PatchView::~PatchView()
{
	//cerr << "Destroying view for " << _patch->path() << endl;
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
PatchView::on_editable_sig(bool editable)
{
	_edit_mode_but->set_active(editable);
	_canvas->lock(!editable);
}


void
PatchView::editable_toggled()
{
	const bool editable = _edit_mode_but->get_active();
	_patch->set_editable(editable);
	_canvas->lock(!editable);
}


void
PatchView::process_toggled()
{
	if (!_enable_signal)
		return;

	App::instance().engine()->set_property(_patch->path(), "ingen:enabled",
			(bool)_process_but->get_active());
}


void
PatchView::poly_changed()
{
	App::instance().engine()->set_property(_patch->path(), "ingen:polyphony",
			_poly_spin->get_value_as_int());
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
PatchView::property_changed(const std::string& predicate, const Raul::Atom& value)
{
	_enable_signal = false;
	if (predicate == "ingen:enabled") {
	   if (value.type() == Atom::BOOL)
		   _process_but->set_active(value.get_bool());
	   else
		   cerr << "WARNING: Bad type for ingen:enabled property: " << value.type() << endl;
	}
	_enable_signal = true;
}


} // namespace GUI
} // namespace Ingen
