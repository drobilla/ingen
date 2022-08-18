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

#include "Style.hpp"

#include "App.hpp"

#include "ingen/URIs.hpp"
#include "ingen/client/PortModel.hpp"

#include <string>

namespace ingen {
namespace gui {

Style::Style(App& app)
    : _app(app)
{
#ifdef INGEN_USE_LIGHT_THEME
	_audio_port_color   = 0xC8E6ABFF;
	_control_port_color = 0xAAC0E6FF;
	_cv_port_color      = 0xACE6E0FF;
	_event_port_color   = 0xE6ABABFF;
	_string_port_color  = 0xD8ABE6FF;
#endif
}

/** Loads settings from the rc file.  Passing no parameter will load from
 * the default location.
 */
void
Style::load_settings(const std::string& filename)
{
	/* ... */
}

/** Saves settings to rc file.  Passing no parameter will save to the
 * default location.
 */
void
Style::save_settings(const std::string& filename)
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
	}

	if (p->is_a(uris.lv2_ControlPort)) {
		return _control_port_color;
	}

	if (p->is_a(uris.lv2_CVPort)) {
		return _cv_port_color;
	}

	if (p->supports(uris.atom_String)) {
		return _string_port_color;
	}

	if (_app.can_control(p)) {
		return _control_port_color;
	}

	if (p->is_a(uris.atom_AtomPort)) {
		return _event_port_color;
	}

	return 0x555555FF;
}

} // namespace gui
} // namespace ingen
