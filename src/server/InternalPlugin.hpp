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

#ifndef INGEN_ENGINE_INTERNALPLUGIN_HPP
#define INGEN_ENGINE_INTERNALPLUGIN_HPP

#include "PluginImpl.hpp"

#include "ingen/URI.hpp"
#include "lilv/lilv.h"
#include "raul/Symbol.hpp"

#define NS_INTERNALS "http://drobilla.net/ns/ingen-internals#"

namespace ingen {

class URIs;

namespace server {

class BlockImpl;
class BufferFactory;
class Engine;
class GraphImpl;

/** Implementation of an Internal plugin.
 */
class InternalPlugin : public PluginImpl
{
public:
	InternalPlugin(URIs& uris, const URI& uri, raul::Symbol symbol);

	BlockImpl* instantiate(BufferFactory&      bufs,
	                       const raul::Symbol& symbol,
	                       bool                polyphonic,
	                       GraphImpl*          parent,
	                       Engine&             engine,
	                       const LilvState*    state) override;

	raul::Symbol symbol() const override { return _symbol; }

private:
	const raul::Symbol _symbol;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_INTERNALPLUGIN_HPP
