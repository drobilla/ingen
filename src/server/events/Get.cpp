/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <utility>

#include "ingen/Interface.hpp"
#include "ingen/Node.hpp"
#include "ingen/Store.hpp"

#include "BlockImpl.hpp"
#include "Broadcaster.hpp"
#include "BufferFactory.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "Get.hpp"
#include "GraphImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {
namespace Events {

Get::Get(Engine&          engine,
         SPtr<Interface>  client,
         int32_t          id,
         SampleCount      timestamp,
         const Raul::URI& uri)
	: Event(engine, client, id, timestamp)
	, _uri(uri)
	, _object(NULL)
	, _plugin(NULL)
{}

bool
Get::pre_process()
{
	std::unique_lock<std::mutex> lock(_engine.store()->mutex());

	if (_uri == "ingen:/plugins") {
		_plugins = _engine.block_factory()->plugins();
		return Event::pre_process_done(Status::SUCCESS);
	} else if (_uri == "ingen:/engine") {
		return Event::pre_process_done(Status::SUCCESS);
	} else if (Node::uri_is_path(_uri)) {
		if ((_object = _engine.store()->get(Node::uri_to_path(_uri)))) {
			const BlockImpl* block = NULL;
			const GraphImpl* graph = NULL;
			const PortImpl*  port  = NULL;
			if ((graph = dynamic_cast<const GraphImpl*>(_object))) {
				_response.put_graph(graph);
			} else if ((block = dynamic_cast<const BlockImpl*>(_object))) {
				_response.put_block(block);
			} else if ((port = dynamic_cast<const PortImpl*>(_object))) {
				_response.put_port(port);
			} else {
				return Event::pre_process_done(Status::BAD_OBJECT_TYPE, _uri);
			}
			return Event::pre_process_done(Status::SUCCESS);
		}
		return Event::pre_process_done(Status::NOT_FOUND, _uri);
	} else if ((_plugin = _engine.block_factory()->plugin(_uri))) {
		_response.put_plugin(_plugin);
		return Event::pre_process_done(Status::SUCCESS);
	} else {
		return Event::pre_process_done(Status::NOT_FOUND, _uri);
	}
}

void
Get::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS && _request_client) {
		if (_uri == "ingen:/plugins") {
			_engine.broadcaster()->send_plugins_to(_request_client.get(), _plugins);
		} else if (_uri == "ingen:/engine") {
			// TODO: Keep a proper RDF model of the engine
			URIs& uris = _engine.world()->uris();
			_request_client->put(
				Raul::URI("ingen:/engine"),
				{ { uris.param_sampleRate,
				    uris.forge.make(int32_t(_engine.driver()->sample_rate())) },
				  { uris.bufsz_maxBlockLength,
				    uris.forge.make(int32_t(_engine.driver()->block_length())) },
				  { uris.ingen_numThreads,
				    uris.forge.make(int32_t(_engine.n_threads())) } });
		} else {
			_response.send(_request_client.get());
		}
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
