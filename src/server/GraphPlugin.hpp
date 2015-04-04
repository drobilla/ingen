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

#ifndef INGEN_ENGINE_GRAPHPLUGIN_HPP
#define INGEN_ENGINE_GRAPHPLUGIN_HPP

#include <string>
#include "PluginImpl.hpp"

namespace Ingen {
namespace Server {

class BlockImpl;

/** Implementation of a Graph plugin.
 *
 * Graphs don't actually work like this yet...
 */
class GraphPlugin : public PluginImpl
{
public:
	GraphPlugin(URIs&               uris,
	            const Raul::URI&    uri,
	            const Raul::Symbol& symbol,
	            const std::string&  name)
		: PluginImpl(uris, Plugin::Graph, uri)
	{}

	BlockImpl* instantiate(BufferFactory&      bufs,
	                       const Raul::Symbol& symbol,
	                       bool                polyphonic,
	                       GraphImpl*          parent,
	                       Engine&             engine)
	{
		return NULL;
	}

	const Raul::Symbol symbol() const { return Raul::Symbol("graph"); }
	const std::string name()    const { return "Ingen Graph"; }

private:
	const std::string _symbol;
	const std::string _name;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_GRAPHPLUGIN_HPP
