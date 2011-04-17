/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_BASE_HPP
#define INGEN_ENGINE_BASE_HPP

#include "raul/SharedPtr.hpp"

namespace Raul { class Maid; }

namespace Ingen {

namespace Shared { class World; }

/**
   The engine which executes the process graph.

   @ingroup interface
*/
class EngineBase
{
public:
	virtual ~EngineBase() {}

	/**
	   Activate the engine.
	*/
	virtual bool activate() = 0;

	/**
	   Deactivate the engine.
	*/
	virtual void deactivate() = 0;

	/**
	   Indicate that a quit is desired

	   This function simply sets a flag which affects the return value of
	   main_iteration, it does not actually force the engine to stop running or
	   block.  The code driving the engine is responsible for stopping and
	   cleaning up when main_iteration returns false.
	*/
	virtual void quit() = 0;

	/**
	   Run a single iteration of the main context.

	   The main context post-processes events and performs housekeeping duties
	   like collecting garbage.  This should be called regularly, e.g. a few
	   times per second.  The return value indicates whether execution should
	   continue; i.e. if false is returned, a quit has been requested and the
	   caller should cease calling main_iteration() and stop the engine.
	*/
	virtual bool main_iteration() = 0;
};

} // namespace Ingen

#endif // INGEN_ENGINE_BASE_HPP
