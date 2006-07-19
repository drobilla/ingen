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
#include "OmFlowCanvas.h"
#include "PatchController.h"
#include "LoadPluginWindow.h"
#include "PatchModel.h"
#include "NewSubpatchWindow.h"
#include "LoadSubpatchWindow.h"
#include "NodeControlWindow.h"
#include "PatchPropertiesWindow.h"
#include "PatchTreeWindow.h"
#include "Controller.h"

namespace Ingenuity {


PatchView::PatchView(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::Box(cobject),
  m_patch(NULL),
  m_canvas(NULL),
  m_enable_signal(true)
{
	property_visible() = false;

	xml->get_widget("patch_canvas_scrolledwindow", m_canvas_scrolledwindow);
	xml->get_widget("patch_zoom_scale", m_zoom_slider);
	xml->get_widget("patch_polyphony_label", m_polyphony_label);
	xml->get_widget("patch_process_checkbutton", m_process_checkbutton);
	
	m_zoom_slider->signal_value_changed().connect(  sigc::mem_fun(this, &PatchView::zoom_changed));
	m_process_checkbutton->signal_toggled().connect(sigc::mem_fun(this, &PatchView::process_toggled));
}


/** Sets the patch controller for this window and initializes everything.
 *
 * This function MUST be called before using the window in any way!
 */
void
PatchView::patch_controller(PatchController* pc)
{
	//m_patch = new PatchController(pm, controller);
	m_patch = pc;
	
	m_canvas = new OmFlowCanvas(pc, 1600*2, 1200*2);
	
	m_canvas_scrolledwindow->add(*m_canvas);
	//m_canvas->show();
	//m_canvas_scrolledwindow->show();

	char txt[4];
	snprintf(txt, 8, "%zd", pc->patch_model()->poly());
	m_polyphony_label->set_text(txt);

	//m_description_window->patch_model(pc->model());
	
	pc->patch_model()->enabled_sig.connect(sigc::mem_fun(this, &PatchView::enable));
	pc->patch_model()->disabled_sig.connect(sigc::mem_fun(this, &PatchView::disable));
}


void
PatchView::show_control_window()
{
	if (m_patch != NULL)
		m_patch->show_control_window();
}


void
PatchView::zoom_changed()
{
	float z = m_zoom_slider->get_value();
	m_canvas->zoom(z);
}


void
PatchView::process_toggled()
{
	if (!m_enable_signal)
		return;

	if (m_process_checkbutton->get_active()) {
		Controller::instance().enable_patch(m_patch->model()->path());
		App::instance().patch_tree()->patch_enabled(m_patch->model()->path());
	} else {
		Controller::instance().disable_patch(m_patch->model()->path());
		App::instance().patch_tree()->patch_disabled(m_patch->model()->path());
	}
}


void
PatchView::enable()
{
	m_enable_signal = false;
	m_process_checkbutton->set_active(true);
	m_enable_signal = true;
}


void
PatchView::disable()
{
	m_enable_signal = false;
	m_process_checkbutton->set_active(false);
	m_enable_signal = true;
}


} // namespace Ingenuity
