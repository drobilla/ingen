/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string>

#include "ingen/URIs.hpp"

#include "App.hpp"
#include "MessagesWindow.hpp"

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

	for (int s = Gtk::STATE_NORMAL; s <= Gtk::STATE_INSENSITIVE; ++s) {
		_textview->modify_base((Gtk::StateType)s, Gdk::Color("#000000"));
		_textview->modify_text((Gtk::StateType)s, Gdk::Color("#EEEEEC"));
	}
}

void
MessagesWindow::init_window(App& app)
{
	Glib::RefPtr<Gtk::TextTag> tag = Gtk::TextTag::create();
	tag->property_foreground() = "#EF2929";
	_tags.emplace(app.uris().log_Error, tag);
	_error_tag = tag;

	tag = Gtk::TextTag::create();
	tag->property_foreground() = "#FCAF3E";
	_tags.emplace(app.uris().log_Warning, tag);

	tag = Gtk::TextTag::create();
	tag->property_foreground() = "#8AE234";
	_tags.emplace(app.uris().log_Trace, tag);

	for (const auto& t : _tags) {
		_textview->get_buffer()->get_tag_table()->add(t.second);
	}
}

void
MessagesWindow::post_error(const string& msg)
{
	Glib::RefPtr<Gtk::TextBuffer> text_buf = _textview->get_buffer();
	text_buf->insert_with_tag(text_buf->end(), msg, _error_tag);
	text_buf->insert(text_buf->end(), "\n");

	if (!_clear_button->is_sensitive()) {
		_clear_button->set_sensitive(true);
	}

	set_urgency_hint(true);
	if (!is_visible()) {
		present();
	}
}

int
MessagesWindow::log(LV2_URID type, const char* fmt, va_list args)
{
	std::lock_guard<std::mutex> lock(_mutex);

#ifdef HAVE_VASPRINTF
	char*     buf = nullptr;
	const int len = vasprintf(&buf, fmt, args);
#else
	char*     buf = g_strdup_vprintf(fmt, args);
	const int len = strlen(buf);
#endif

	_stream << type << ' ' << buf << '\0';
	free(buf);

	return len;
}

void
MessagesWindow::flush()
{
	while (true) {
		LV2_URID    type;
		std::string line;
		{
			std::lock_guard<std::mutex> lock(_mutex);
			if (!_stream.rdbuf()->in_avail()) {
				return;
			}
			_stream >> type;
			std::getline(_stream, line, '\0');
		}

		Glib::RefPtr<Gtk::TextBuffer> text_buf = _textview->get_buffer();

		auto t = _tags.find(type);
		if (t != _tags.end()) {
			text_buf->insert_with_tag(text_buf->end(), line, t->second);
		} else {
			text_buf->insert(text_buf->end(), line);
		}
	}

	if (!_clear_button->is_sensitive()) {
		_clear_button->set_sensitive(true);
	}
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
