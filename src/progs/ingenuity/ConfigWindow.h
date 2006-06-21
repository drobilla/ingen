/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */


#ifndef CONFIGWINDOW_H
#define CONFIGWINDOW_H

#include "PluginModel.h"
#include "Configuration.h"
#include <list>
#include <libglademm/xml.h>
#include <libglademm.h>
#include <gtkmm.h>

using std::list;
using LibOmClient::PluginModel;

namespace OmGtk {
	

/** 'Configuration' window.
 *
 * Loaded by glade as a derived object.
 *
 * \ingroup OmGtk
 */
class ConfigWindow : public Gtk::Window
{
public:
	ConfigWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);

	void configuration(Configuration* sm);

private:
	void save_clicked();
	void cancel_clicked();
	void ok_clicked();
	
	Configuration* m_configuration;
	
	Gtk::Entry*       m_path_entry;
	Gtk::Button*      m_save_button;
	Gtk::Button*      m_cancel_button;
	Gtk::Button*      m_ok_button;
};


} // namespace OmGtk

#endif // CONFIGWINDOW_H
