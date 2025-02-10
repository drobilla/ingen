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

#ifndef INGEN_GUI_MESSAGESWINDOW_HPP
#define INGEN_GUI_MESSAGESWINDOW_HPP

#include "Window.hpp"

#include <lv2/urid/urid.h>

#include <glibmm/refptr.h>
#include <gtkmm/texttag.h>

#include <cstdarg>
#include <map>
#include <mutex>
#include <sstream>
#include <string>

namespace Gtk {
class Builder;
class Button;
class TextView;
} // namespace Gtk

namespace ingen::gui {

/** Messages Window.
 *
 * Loaded from XML as a derived object.
 * This is shown when errors occur (e.g. during graph loading).
 *
 * \ingroup GUI
 */
class MessagesWindow : public Window
{
public:
	MessagesWindow(BaseObjectType*                   cobject,
	               const Glib::RefPtr<Gtk::Builder>& xml);

	void init_window(App& app) override;

	int log(LV2_URID type, const char* fmt, va_list args);
	void flush();

	void post_error(const std::string& msg);

private:
	void clear_clicked();

	std::mutex        _mutex;
	std::stringstream _stream;
	Gtk::TextView*    _textview{nullptr};
	Gtk::Button*      _clear_button{nullptr};
	Gtk::Button*      _close_button{nullptr};

	Glib::RefPtr<Gtk::TextTag>                      _error_tag;
	std::map< LV2_URID, Glib::RefPtr<Gtk::TextTag> > _tags;
};

} // namespace ingen::gui

#endif // INGEN_GUI_MESSAGESWINDOW_HPP
