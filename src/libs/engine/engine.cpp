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

#include "engine.h"
#include "Engine.h"
#include "QueuedEngineInterface.h"
#include "tuning.h"

namespace Ingen {


Engine*
new_engine()
{
	return new Engine();
}


QueuedEngineInterface*
new_queued_engine_interface(Engine& engine)
{
	return new QueuedEngineInterface(engine,
			Ingen::event_queue_size, Ingen::event_queue_size);
}


} // namespace Ingen

