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

#include "LoadPluginsEvent.h"
#include "Responder.h"
#include "Engine.h"
#include "NodeFactory.h"
#include "ClientBroadcaster.h"

#include <iostream>
using std::cerr;

namespace Ingen {


LoadPluginsEvent::LoadPluginsEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp)
: QueuedEvent(engine, responder, timestamp)
{
}

void
LoadPluginsEvent::pre_process()
{
	_engine.node_factory()->load_plugins();
	
	// FIXME: send the changes (added and removed plugins) instead of the entire list each time
	
	// Take a copy to send in the post processing thread (to avoid problems
	// because std::list isn't thread safe)
	_plugins = _engine.node_factory()->plugins();
	
	QueuedEvent::pre_process();
}

void
LoadPluginsEvent::post_process()
{
	_responder->respond_ok();
}


} // namespace Ingen

