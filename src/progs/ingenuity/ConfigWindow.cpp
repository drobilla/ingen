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

#include "ConfigWindow.h"
#include <iostream>
#include <cassert>
#include <algorithm>
#include <cctype>
#include "NodeModel.h"
using std::cout; using std::cerr; using std::endl;


namespace Ingenuity {

ConfigWindow::ConfigWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::Window(cobject),
  m_configuration(NULL)
{
	xml->get_widget("config_path_entry", m_path_entry);
	xml->get_widget("config_save_button", m_save_button);
	xml->get_widget("config_cancel_button", m_cancel_button);
	xml->get_widget("config_ok_button", m_ok_button);
	
	m_save_button->signal_clicked().connect(  sigc::mem_fun(this, &ConfigWindow::save_clicked));
	m_cancel_button->signal_clicked().connect(sigc::mem_fun(this, &ConfigWindow::cancel_clicked));
	m_ok_button->signal_clicked().connect(    sigc::mem_fun(this, &ConfigWindow::ok_clicked));
}


/** Sets the state manager for this window and initializes everything.
 *
 * This function MUST be called before using the window in any way!
 */
void
ConfigWindow::configuration(Configuration* sm)
{
	m_configuration = sm;
	m_path_entry->set_text(sm->patch_path());
}



///// Event Handlers //////


void
ConfigWindow::save_clicked()
{
	m_configuration->patch_path(m_path_entry->get_text());
	m_configuration->apply_settings();
	m_configuration->save_settings();
}


void
ConfigWindow::cancel_clicked()
{
	hide();
}


void
ConfigWindow::ok_clicked()
{
	m_configuration->patch_path(m_path_entry->get_text());
	m_configuration->apply_settings();
	hide();
}


} // namespace Ingenuity
