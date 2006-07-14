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

#ifndef MESSAGESWINDOW_H
#define MESSAGESWINDOW_H

#include <string>
#include <gtkmm.h>
#include <libglademm/xml.h>
using std::string;


namespace OmGtk {
	

/** Messages Window.
 *
 * Loaded by libglade as a derived object.
 * This is shown when errors occur (ie during patch loading).
 *
 * \ingroup OmGtk
 */
class MessagesWindow : public Gtk::Window
{
public:
	MessagesWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade);

	void post(const string& str);

private:
	void clear_clicked();
	void close_clicked();

	Gtk::TextView* m_textview;
	Gtk::Button*   m_clear_button;
	Gtk::Button*   m_close_button;
};


} // namespace OmGtk

#endif // MESSAGESWINDOW_H
