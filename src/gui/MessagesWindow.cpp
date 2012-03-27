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

#include "MessagesWindow.hpp"
#include <string>

namespace Ingen {
namespace GUI {
using std::string;

MessagesWindow::MessagesWindow(BaseObjectType*                   cobject,
                               const Glib::RefPtr<Gtk::Builder>& xml)
	: Window(cobject)
{
	xml->get_widget("messages_textview", _textview);
	xml->get_widget("messages_clear_button", _clear_button);
	xml->get_widget("messages_close_button", _close_button);

	_clear_button->signal_clicked().connect(sigc::mem_fun(this, &MessagesWindow::clear_clicked));
	_close_button->signal_clicked().connect(sigc::mem_fun(this, &Window::hide));
}

void
MessagesWindow::post(const string& msg)
{
	Glib::RefPtr<Gtk::TextBuffer> text_buf = _textview->get_buffer();
	text_buf->insert(text_buf->end(), msg);
	text_buf->insert(text_buf->end(), "\n");

	if (!_clear_button->is_sensitive())
		_clear_button->set_sensitive(true);
}

void
MessagesWindow::clear_clicked()
{
	Glib::RefPtr<Gtk::TextBuffer> text_buf = _textview->get_buffer();
	text_buf->erase(text_buf->begin(), text_buf->end());
	_clear_button->set_sensitive(false);
}

} // namespace GUI
} // namespace Ingen
