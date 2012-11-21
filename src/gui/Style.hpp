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

#ifndef INGEN_GUI_STYLE_HPP
#define INGEN_GUI_STYLE_HPP

#include <stdint.h>

#include <string>

#include "raul/SharedPtr.hpp"

namespace Ingen { namespace Client { class PortModel; } }

namespace Ingen {
namespace GUI {

class App;
class Port;

class Style
{
public:
	explicit Style(App& app);
	~Style();

	void load_settings(std::string filename = "");
	void save_settings(std::string filename = "");

	void apply_settings();

	uint32_t get_port_color(const Client::PortModel* p);

private:
	App& _app;

	uint32_t _audio_port_color;
	uint32_t _control_port_color;
	uint32_t _cv_port_color;
	uint32_t _event_port_color;
	uint32_t _string_port_color;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_STYLE_HPP

