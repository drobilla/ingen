/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Get.hpp"

#include "BlockImpl.hpp"
#include "Broadcaster.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "PortImpl.hpp"

#include <ingen/Forge.hpp>
#include <ingen/Interface.hpp>
#include <ingen/Message.hpp>
#include <ingen/Node.hpp>
#include <ingen/Properties.hpp>
#include <ingen/Status.hpp>
#include <ingen/Store.hpp>
#include <ingen/URI.hpp>
#include <ingen/URIs.hpp>
#include <ingen/World.hpp>
#include <ingen/paths.hpp>

#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>

namespace ingen::server::events {

Get::Get(Engine&                           engine,
         const std::shared_ptr<Interface>& client,
         SampleCount                       timestamp,
         const ingen::Get&                 msg)
	: Event(engine, client, msg.seq, timestamp)
	, _msg(msg)
{}

bool
Get::pre_process(PreProcessContext&)
{
	const std::lock_guard<Store::Mutex> lock{_engine.store()->mutex()};

	const auto& uri = _msg.subject;
	if (uri == "ingen:/plugins") {
		_plugins = _engine.block_factory()->plugins();
		return Event::pre_process_done(Status::SUCCESS);
	}

	if (uri == "ingen:/engine") {
		return Event::pre_process_done(Status::SUCCESS);
	}

	if (uri_is_path(uri)) {
		if ((_object = _engine.store()->get(uri_to_path(uri)))) {
			const BlockImpl* block = nullptr;
			const GraphImpl* graph = nullptr;
			const PortImpl*  port  = nullptr;
			if ((graph = dynamic_cast<const GraphImpl*>(_object))) {
				_response.put_graph(graph);
			} else if ((block = dynamic_cast<const BlockImpl*>(_object))) {
				_response.put_block(block);
			} else if ((port = dynamic_cast<const PortImpl*>(_object))) {
				_response.put_port(port);
			} else {
				return Event::pre_process_done(Status::BAD_OBJECT_TYPE, uri);
			}
			return Event::pre_process_done(Status::SUCCESS);
		}
		return Event::pre_process_done(Status::NOT_FOUND, uri);
	}

	if ((_plugin = _engine.block_factory()->plugin(uri))) {
		_response.put_plugin(_plugin);
		return Event::pre_process_done(Status::SUCCESS);
	}

	return Event::pre_process_done(Status::NOT_FOUND, uri);
}

void
Get::execute(RunContext&)
{}

void
Get::post_process()
{
	const Broadcaster::Transfer t{*_engine.broadcaster()};
	if (respond() == Status::SUCCESS && _request_client) {
		if (_msg.subject == "ingen:/plugins") {
			_engine.broadcaster()->send_plugins_to(_request_client.get(), _plugins);
		} else if (_msg.subject == "ingen:/engine") {
			// TODO: Keep a proper RDF model of the engine
			URIs&      uris  = _engine.world().uris();
			Properties props = {
				{ uris.param_sampleRate,
				  uris.forge.make(static_cast<int32_t>(_engine.sample_rate())) },
				{ uris.bufsz_maxBlockLength,
				  uris.forge.make(static_cast<int32_t>(_engine.block_length())) },
				{ uris.ingen_numThreads,
				  uris.forge.make(static_cast<int32_t>(_engine.n_threads())) } };

			const Properties load_props = _engine.load_properties();
			props.insert(load_props.begin(), load_props.end());
			_request_client->put(URI("ingen:/engine"), props);
		} else {
			_response.send(*_request_client);
		}
	}
}

} // namespace ingen::server::events
