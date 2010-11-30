/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#include "Configuration.hpp"
#include <cstdlib>
#include <cassert>
#include <fstream>
#include <map>
#include "raul/log.hpp"
#include "client/PortModel.hpp"
#include "client/PluginModel.hpp"
#include "serialisation/Parser.hpp"
#include "shared/LV2URIMap.hpp"
#include "flowcanvas/Port.hpp"
#include "App.hpp"
#include "Port.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace GUI {

using namespace Ingen::Client;


Configuration::Configuration()
	// Colours from  the Tango palette with modified V and alpha
	: _name_style(HUMAN)
	, _audio_port_color(  0x244678C0) // Blue
	, _control_port_color(0x4A8A0EC0) // Green
	, _event_port_color(  0x960909C0) // Red
	, _string_port_color( 0x5C3566C0) // Plum
	, _value_port_color(  0xBABDB6C0) // Aluminum
{
}


Configuration::~Configuration()
{
}


/** Loads settings from the rc file.  Passing no parameter will load from
 * the default location.
 */
void
Configuration::load_settings(string filename)
{
	/* ... */
}


/** Saves settings to rc file.  Passing no parameter will save to the
 * default location.
 */
void
Configuration::save_settings(string filename)
{
	/* ... */
}


/** Applies the current loaded settings to whichever parts of the app
 * need updating.
 */
void
Configuration::apply_settings()
{
	/* ... */
}


uint32_t
Configuration::get_port_color(const PortModel* p)
{
	assert(p != NULL);
	const Shared::LV2URIMap& uris = App::instance().uris();
	if (p->is_a(Shared::PortType::AUDIO)) {
		return _audio_port_color;
	} else if (p->supports(uris.atom_String)) {
		return _string_port_color;
	} else if (App::instance().can_control(p)) {
		return _control_port_color;
	} else if (p->is_a(Shared::PortType::EVENTS) || p->is_a(Shared::PortType::MESSAGE)) {
		return _event_port_color;
	}

	warn << "[Configuration] No known port type for " << p->path() << endl;
	return 0x666666FF;
}


} // namespace GUI
} // namespace Ingen
