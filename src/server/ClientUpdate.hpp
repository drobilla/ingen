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

#include "ingen/Properties.hpp"
#include "ingen/Resource.hpp"
#include "ingen/URI.hpp"
#include "raul/Path.hpp"

#include <string>
#include <vector>

namespace ingen {

class Interface;
class URIs;

namespace server {

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
	void put(const URI&        uri,
	         const Properties& props,
	         Resource::Graph   ctx = Resource::Graph::DEFAULT);

	void put_port(const PortImpl* port);
	void put_block(const BlockImpl* block);
	void put_graph(const GraphImpl* graph);
	void put_plugin(PluginImpl* plugin);
	void put_preset(const URIs&        uris,
	                const URI&         plugin,
	                const URI&         preset,
	                const std::string& label);

	void del(const URI& subject);

	void send(Interface& dest);

	struct Put {
		URI             uri;
		Properties      properties;
		Resource::Graph ctx;
	};

	struct Connect {
		raul::Path tail;
		raul::Path head;
	};

	std::vector<URI>     dels;
	std::vector<Put>     puts;
	std::vector<Connect> connects;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_CLIENTUPDATE_HPP
