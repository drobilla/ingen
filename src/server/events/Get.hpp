/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_EVENTS_GET_HPP
#define INGEN_EVENTS_GET_HPP

#include <vector>

#include "Event.hpp"
#include "BlockFactory.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {

class BlockImpl;
class GraphImpl;
class PluginImpl;
class PortImpl;

namespace Events {

/** A request from a client to send an object.
 *
 * \ingroup engine
 */
class Get : public Event
{
public:
	Get(Engine&          engine,
	    SPtr<Interface>  client,
	    int32_t          id,
	    SampleCount      timestamp,
	    const Raul::URI& uri);

	bool pre_process();
	void execute(ProcessContext& context) {}
	void post_process();

	/** A sequence of puts and connects to respond to client with.
	 * This is constructed in the pre_process() and later sent in
	 * post_process() to avoid the need to lock.
	 *
	 * Ideally events (both server and client) would always be in a standard
	 * message format so the Ingen protocol went the whole way through the
	 * system, but for now things are controlled procedurally through
	 * Interface, so this interim structure is necessary.
	 */
	struct Response {
		void put(const Raul::URI&            uri,
		         const Resource::Properties& props,
		         Resource::Graph             ctx=Resource::Graph::DEFAULT);

		void put_port(const PortImpl* port);
		void put_block(const BlockImpl* block);
		void put_graph(const GraphImpl* graph);

		void send(Interface* dest);

		struct Put {
			Raul::URI            uri;
			Resource::Properties properties;
			Resource::Graph      ctx;
		};

		struct Connect {
			Raul::Path tail;
			Raul::Path head;
		};

		std::vector<Put>     puts;
		std::vector<Connect> connects;
	};

private:
	const Raul::URI       _uri;
	const Node*           _object;
	const PluginImpl*     _plugin;
	BlockFactory::Plugins _plugins;
	Response              _response;
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_GET_HPP
