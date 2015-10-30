/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_CLIENTUPDATE_HPP
#define INGEN_ENGINE_CLIENTUPDATE_HPP

#include <string>
#include <vector>

#include "ingen/Resource.hpp"
#include "raul/Path.hpp"
#include "raul/URI.hpp"

namespace Ingen {

class Interface;
class URIs;

namespace Server {

class PortImpl;
class BlockImpl;
class GraphImpl;
class PluginImpl;

/** A sequence of puts/connects/deletes to update clients.
 *
 * Events like Get construct this in pre_process() and later send it in
 * post_process() to avoid the need to lock.
 */
struct ClientUpdate {
	void put(const Raul::URI&            uri,
	         const Resource::Properties& props,
	         Resource::Graph             ctx=Resource::Graph::DEFAULT);

	void put_port(const PortImpl* port);
	void put_block(const BlockImpl* block);
	void put_graph(const GraphImpl* graph);
	void put_plugin(PluginImpl* plugin);
	void put_preset(const URIs&        uris,
	                const Raul::URI&   plugin,
	                const Raul::URI&   preset,
	                const std::string& label);

	void del(const Raul::URI& subject);

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

	std::vector<Raul::URI> dels;
	std::vector<Put>       puts;
	std::vector<Connect>   connects;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_CLIENTUPDATE_HPP
