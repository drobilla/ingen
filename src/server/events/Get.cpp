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

#include "ingen/ClientInterface.hpp"

#include "ClientBroadcaster.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "Get.hpp"
#include "ObjectSender.hpp"
#include "PluginImpl.hpp"

using namespace Raul;

namespace Ingen {
namespace Server {
namespace Events {

Get::Get(Engine&          engine,
         ClientInterface* client,
         int32_t          id,
         SampleCount      timestamp,
         const URI&       uri)
	: Event(engine, client, id, timestamp)
	, _uri(uri)
	, _object(NULL)
	, _plugin(NULL)
	, _lock(engine.engine_store()->lock(), Glib::NOT_LOCK)
{
}

void
Get::pre_process()
{
	_lock.acquire();

	if (_uri == "ingen:plugins") {
		_plugins = _engine.node_factory()->plugins();
	} else if (Path::is_valid(_uri.str())) {
		_object = _engine.engine_store()->find_object(Path(_uri.str()));
	} else {
		_plugin = _engine.node_factory()->plugin(_uri);
	}

	Event::pre_process();
}

void
Get::post_process()
{
	if (_uri == "ingen:plugins") {
		respond_ok();
		if (_request_client) {
			_engine.broadcaster()->send_plugins_to(_request_client, _plugins);
		}
	} else if (!_object && !_plugin) {
		respond_error("Unable to find object requested.");
	} else if (_request_client) {
		respond_ok();
		if (_request_client) {
			if (_object) {
				ObjectSender::send_object(_request_client, _object, true);
			} else if (_plugin) {
				_request_client->put(_uri, _plugin->properties());
			}
		}
	} else {
		respond_error("Unable to find client to send object.");
	}

	_lock.release();
}

} // namespace Server
} // namespace Ingen
} // namespace Events

