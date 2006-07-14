/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
#include "Om.h"
#include "OmApp.h"
#include "NodeFactory.h"

#include <iostream>
using std::cerr;

namespace Om {


LoadPluginsEvent::LoadPluginsEvent(CountedPtr<Responder> responder, samplecount timestamp)
: QueuedEvent(responder, timestamp)
{
	cerr << "LOADING PLUGINS\n";
	om->node_factory()->load_plugins();
}


void
LoadPluginsEvent::post_process()
{
	_responder->respond_ok();
}


} // namespace Om

