/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
#include <iostream>
#include <fstream>
#include <map>
#include "client/PortModel.hpp"
#include "client/PluginModel.hpp"
#include "client/PatchModel.hpp"
#include "serialisation/Loader.hpp"
#include "App.hpp"

using std::cerr; using std::cout; using std::endl;
using std::map; using std::string;
using Ingen::Client::PatchModel;

namespace Ingen {
namespace GUI {

using namespace Ingen::Client;


Configuration::Configuration()
	// Colours from  the Tango palette with modified V and alpha
	: _audio_port_color(  0x244678C0)
	, _control_port_color(0x4A8A0EC0)
	, _midi_port_color(   0x960909C0)
	, _osc_port_color(    0x5C3566C0)
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

	if (p->type().is_control()) {
		return _control_port_color;
	} else if (p->type().is_audio()) {
		return _audio_port_color;
	} else if (p->type().is_midi()) {
		return _midi_port_color;
	} else if (p->type().is_osc()) {
		return _osc_port_color;
	}
	
	cerr << "[Configuration] Unknown port type " << p->type().uri()
		<< ", port will appear black." << endl;
	
	return 0x000000FF;
}


} // namespace GUI
} // namespace Ingen
