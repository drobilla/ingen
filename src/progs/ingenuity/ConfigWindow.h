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


#ifndef CONFIGWINDOW_H
#define CONFIGWINDOW_H

#include "PluginModel.h"
#include "Configuration.h"
#include <list>
#include <libglademm/xml.h>
#include <libglademm.h>
#include <gtkmm.h>

using std::list;
using Ingen::Client::PluginModel;

namespace Ingenuity {
	

/** 'Configuration' window.
 *
 * Loaded by glade as a derived object.
 *
 * \ingroup Ingenuity
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
	
	Configuration* _configuration;
	
	Gtk::Entry*       _path_entry;
	Gtk::Button*      _save_button;
	Gtk::Button*      _cancel_button;
	Gtk::Button*      _ok_button;
};


} // namespace Ingenuity

#endif // CONFIGWINDOW_H
