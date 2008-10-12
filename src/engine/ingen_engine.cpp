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

#include "config.h"

#include <string>
#include <raul/Process.hpp>
#include "ingen_engine.hpp"
#include "Engine.hpp"
#include "tuning.hpp"
#include "util.hpp"

namespace Ingen {

Engine*
new_engine(Ingen::Shared::World* world)
{
	set_denormal_flags();
	return new Engine(world);
}


bool
launch_osc_engine(int port)
{
	char port_str[6];
	snprintf(port_str, 6, "%u", port);
	const std::string cmd = std::string("ingen -e --engine-port=").append(port_str);

	if (Raul::Process::launch(cmd)) {
		return true;
	} else {
		std::cerr << "Failed to launch engine process." << std::endl;
		return false;
	}
}


} // namespace Ingen

