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

#include "RequestPluginEvent.h"
#include <string>
#include "Responder.h"
#include "Engine.h"
#include "interface/ClientInterface.h"
#include "TypedPort.h"
#include "ObjectStore.h"
#include "ClientBroadcaster.h"
#include "NodeFactory.h"
#include "Plugin.h"

using std::string;

namespace Ingen {


RequestPluginEvent::RequestPluginEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& uri)
: QueuedEvent(engine, responder, timestamp),
  _uri(uri),
  _plugin(NULL)
{
}


void
RequestPluginEvent::pre_process()
{
	_client = _engine.broadcaster()->client(_responder->client_key());
	_plugin = _engine.node_factory()->plugin(_uri);

	QueuedEvent::pre_process();
}


void
RequestPluginEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);
	assert(_time >= start && _time <= end);
}


void
RequestPluginEvent::post_process()
{
	if (!_plugin) {
		_responder->respond_error("Unable to find plugin requested.");
	
	} else if (_client) {

		_responder->respond_ok();
		assert(_plugin->uri() == _uri);
		_client->new_plugin(_uri, _plugin->type_uri(), _plugin->name());

	} else {
		_responder->respond_error("Unable to find client to send plugin.");
	}
}


} // namespace Ingen

