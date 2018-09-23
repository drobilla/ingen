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

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>

#include "ganv/Port.hpp"
#include "ingen/Log.hpp"
#include "ingen/Parser.hpp"
#include "ingen/client/PluginModel.hpp"
#include "ingen/client/PortModel.hpp"

#include "App.hpp"
#include "Style.hpp"
#include "Port.hpp"

namespace ingen {
namespace gui {

using namespace ingen::client;

Style::Style(App& app)
	// Colours from the Tango palette with modified V
	: _app(app)
#ifdef INGEN_USE_LIGHT_THEME
	, _audio_port_color(0xC8E6ABFF) // Green
	, _control_port_color(0xAAC0E6FF) // Blue
	, _cv_port_color(0xACE6E0FF) // Teal (between audio and control)
	, _event_port_color(0xE6ABABFF) // Red
	, _string_port_color(0xD8ABE6FF) // Plum
#else
	, _audio_port_color(0x4A8A0EFF) // Green
	, _control_port_color(0x244678FF) // Blue
	, _cv_port_color(0x248780FF) // Teal (between audio and control)
	, _event_port_color(0x960909FF) // Red
	, _string_port_color(0x5C3566FF) // Plum
#endif
{
}

/** Loads settings from the rc file.  Passing no parameter will load from
 * the default location.
 */
void
Style::load_settings(std::string filename)
{
	/* ... */
}

/** Saves settings to rc file.  Passing no parameter will save to the
 * default location.
 */
void
Style::save_settings(std::string filename)
{
	/* ... */
}

/** Applies the current loaded settings to whichever parts of the app
 * need updating.
 */
void
Style::apply_settings()
{
	/* ... */
}

uint32_t
Style::get_port_color(const client::PortModel* p)
{
	const URIs& uris = _app.uris();
	if (p->is_a(uris.lv2_AudioPort)) {
		return _audio_port_color;
	} else if (p->is_a(uris.lv2_ControlPort)) {
		return _control_port_color;
	} else if (p->is_a(uris.lv2_CVPort)) {
		return _cv_port_color;
	} else if (p->supports(uris.atom_String)) {
		return _string_port_color;
	} else if (_app.can_control(p)) {
		return _control_port_color;
	} else if (p->is_a(uris.atom_AtomPort)) {
		return _event_port_color;
	}

	return 0x555555FF;
}

} // namespace gui
} // namespace ingen
