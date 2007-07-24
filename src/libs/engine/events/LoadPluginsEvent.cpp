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

#include "LoadPluginsEvent.hpp"
#include "interface/Responder.hpp"
#include "Engine.hpp"
#include "NodeFactory.hpp"
#include "ClientBroadcaster.hpp"
#include "QueuedEventSource.hpp"

#include <iostream>
using std::cerr;

namespace Ingen {


LoadPluginsEvent::LoadPluginsEvent(Engine& engine, SharedPtr<Shared::Responder> responder, SampleCount timestamp, QueuedEventSource* source)
: QueuedEvent(engine, responder, timestamp, true, source)
{
	/* Not sure why this has to be blocking, but it fixes some nasty bugs.. */
}

void
LoadPluginsEvent::pre_process()
{
	_engine.node_factory()->load_plugins();
	
	QueuedEvent::pre_process();
}

void
LoadPluginsEvent::post_process()
{
	if (_source)
		_source->unblock();
	
	_responder->respond_ok();
}


} // namespace Ingen

