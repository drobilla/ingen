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

#include <string>
#include "interface/ClientInterface.hpp"
#include "RequestPluginEvent.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "PortImpl.hpp"
#include "EngineStore.hpp"
#include "ClientBroadcaster.hpp"
#include "NodeFactory.hpp"
#include "PluginImpl.hpp"
#include "ProcessContext.hpp"

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
	_plugin = _engine.node_factory()->plugin(_uri);

	QueuedEvent::pre_process();
}


void
RequestPluginEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);
	assert(_time >= context.start() && _time <= context.end());
}


void
RequestPluginEvent::post_process()
{
	if (!_plugin) {
		_responder->respond_error("Unable to find plugin requested.");
	
	} else if (_responder->client()) {

		_responder->respond_ok();
		assert(_plugin->uri() == _uri);
		_responder->client()->new_plugin(_uri, _plugin->type_uri(), _plugin->symbol(), _plugin->name());

	} else {
		_responder->respond_error("Unable to find client to send plugin.");
	}
}


} // namespace Ingen

