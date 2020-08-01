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

#include "PluginImpl.hpp"

#include <string>

namespace ingen {
namespace server {

class BlockImpl;

/** Implementation of a Graph plugin.
 *
 * Graphs don't actually work like this yet...
 */
class GraphPlugin : public PluginImpl
{
public:
	GraphPlugin(URIs&               uris,
	            const URI&          uri,
	            const Raul::Symbol& symbol,
	            const std::string&  name)
		: PluginImpl(uris, uris.ingen_Graph.urid_atom(), uri)
	{}

	BlockImpl* instantiate(BufferFactory&      bufs,
	                       const Raul::Symbol& symbol,
	                       bool                polyphonic,
	                       GraphImpl*          parent,
	                       Engine&             engine,
	                       const LilvState*    state) override
	{
		return nullptr;
	}

	Raul::Symbol       symbol() const override { return Raul::Symbol("graph"); }
	static std::string name() { return "Ingen Graph"; }

private:
	const std::string _symbol;
	const std::string _name;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_GRAPHPLUGIN_HPP
