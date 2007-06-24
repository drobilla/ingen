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

#ifndef INGEN_ENGINE_H
#define INGEN_ENGINE_H

#include <raul/SharedPtr.h>

namespace Ingen {

class Engine;
namespace Shared { class EngineInterface; }


extern "C" {

	//void run(int argc, char** argv);

	/** Create a new engine in this process */
	Engine* new_engine();

	/** Launch an OSC engine as a completely separate process
	 * \return true if successful
	 */
	bool launch_osc_engine(int port);
}


} // namespace Ingen

#endif // INGEN_ENGINE_H

