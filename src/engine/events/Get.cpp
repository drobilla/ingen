/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#include "Get.hpp"
#include "interface/ClientInterface.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "ClientBroadcaster.hpp"
#include "PatchImpl.hpp"
#include "NodeImpl.hpp"
#include "PortImpl.hpp"
#include "ObjectSender.hpp"
#include "ProcessContext.hpp"

using namespace Raul;

namespace Ingen {
namespace Events {


Get::Get(
		Engine&              engine,
		SharedPtr<Responder> responder,
		SampleCount          timestamp,
		const URI&           uri)
	: QueuedEvent(engine, responder, timestamp)
	, _uri(uri)
	, _object(NULL)
	, _plugin(NULL)
{
}


void
Get::pre_process()
{
	if (Path::is_valid(_uri.str()))
		_object = _engine.engine_store()->find_object(Path(_uri.str()));
	else
		_plugin = _engine.node_factory()->plugin(_uri);

	QueuedEvent::pre_process();
}


void
Get::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);
	assert(_time >= context.start() && _time <= context.end());
}


void
Get::post_process()
{
	if (!_object && !_plugin) {
		_responder->respond_error("Unable to find object requested.");
	} else if (_responder->client()) {
		_responder->respond_ok();
		if (_object)
			_responder->client()->put(_uri, _object->properties());
		else if (_plugin)
			_responder->client()->put(_uri, _plugin->properties());
	} else {
		_responder->respond_error("Unable to find client to send object.");
	}
}


} // namespace Ingen
} // namespace Events

