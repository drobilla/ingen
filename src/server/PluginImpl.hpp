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

#ifndef INGEN_ENGINE_PLUGINIMPL_HPP
#define INGEN_ENGINE_PLUGINIMPL_HPP

#include <cstdlib>
#include <string>

#include <boost/utility.hpp>

#include "ingen/Plugin.hpp"
#include "ingen/Resource.hpp"

namespace Ingen {

namespace Shared { class URIs; }

namespace Server {

class PatchImpl;
class NodeImpl;
class Engine;
class BufferFactory;

/** Implementation of a plugin (internal code, or a loaded shared library).
 *
 * Conceptually, a Node is an instance of this.
 */
class PluginImpl : public Plugin
                 , public boost::noncopyable
{
public:
	PluginImpl(Ingen::Shared::URIs& uris,
	           Type                 type,
	           const std::string&   uri)
		: Plugin(uris, uri)
		, _type(type)
	{}

	virtual NodeImpl* instantiate(BufferFactory&     bufs,
	                              const std::string& name,
	                              bool               polyphonic,
	                              PatchImpl*         parent,
	                              Engine&            engine) = 0;

	virtual const std::string symbol() const = 0;

	Plugin::Type  type() const         { return _type; }
	void          type(Plugin::Type t) { _type = t; }

protected:
	Plugin::Type _type;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_PLUGINIMPL_HPP

