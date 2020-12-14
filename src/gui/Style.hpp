/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_STYLE_HPP
#define INGEN_GUI_STYLE_HPP

#include <cstdint>
#include <string>

namespace ingen {

namespace client {
class PortModel;
} // namespace client

namespace gui {

class App;

class Style
{
public:
	explicit Style(App& app);

	void load_settings(const std::string& filename = "");
	void save_settings(const std::string& filename = "");

	void apply_settings();

	uint32_t get_port_color(const client::PortModel* p);

private:
	App& _app;

	uint32_t _audio_port_color;
	uint32_t _control_port_color;
	uint32_t _cv_port_color;
	uint32_t _event_port_color;
	uint32_t _string_port_color;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_STYLE_HPP
