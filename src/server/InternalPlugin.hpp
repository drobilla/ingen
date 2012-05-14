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

#ifndef INGEN_ENGINE_INTERNALPLUGIN_HPP
#define INGEN_ENGINE_INTERNALPLUGIN_HPP

#include <cstdlib>
#include <string>

#include <boost/utility.hpp>
#include <glibmm/module.h>

#include "PluginImpl.hpp"

#define NS_INTERNALS "http://drobilla.net/ns/ingen-internals#"

namespace Ingen {
namespace Server {

class NodeImpl;
class BufferFactory;

/** Implementation of an Internal plugin.
 */
class InternalPlugin : public PluginImpl
{
public:
	InternalPlugin(Shared::URIs&      uris,
	               const std::string& uri,
	               const std::string& symbol);

	NodeImpl* instantiate(BufferFactory&     bufs,
	                      const std::string& name,
	                      bool               polyphonic,
	                      PatchImpl*         parent,
	                      Engine&            engine);

	const std::string symbol() const { return _symbol; }

private:
	const std::string _symbol;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_INTERNALPLUGIN_HPP

