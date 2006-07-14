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

#include "MessagesWindow.h"
#include <string>

namespace OmGtk {
using std::string;


MessagesWindow::MessagesWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml)
: Gtk::Window(cobject)
{
	glade_xml->get_widget("messages_textview", m_textview);
	glade_xml->get_widget("messages_clear_button", m_clear_button);
	glade_xml->get_widget("messages_close_button", m_close_button);

	m_clear_button->signal_clicked().connect(sigc::mem_fun(this, &MessagesWindow::clear_clicked));
	m_close_button->signal_clicked().connect(sigc::mem_fun(this, &MessagesWindow::close_clicked));
}


void
MessagesWindow::post(const string& msg)
{
	Glib::RefPtr<Gtk::TextBuffer> text_buf = m_textview->get_buffer();
	text_buf->insert(text_buf->end(), msg);
	text_buf->insert(text_buf->end(), "\n");

	if (!m_clear_button->is_sensitive())
		m_clear_button->set_sensitive(true);
}


void
MessagesWindow::close_clicked()
{
	hide();
}


void
MessagesWindow::clear_clicked()
{
	Glib::RefPtr<Gtk::TextBuffer> text_buf = m_textview->get_buffer();
	text_buf->erase(text_buf->begin(), text_buf->end());
	m_clear_button->set_sensitive(false);
}


} // namespace OmGtk
