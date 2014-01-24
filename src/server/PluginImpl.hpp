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

#include <boost/utility.hpp>

#include "ingen/Plugin.hpp"
#include "ingen/Resource.hpp"
#include "raul/Symbol.hpp"
#include "raul/URI.hpp"

namespace Ingen {

class URIs;

namespace Server {

class BlockImpl;
class BufferFactory;
class Engine;
class GraphImpl;

/** Implementation of a plugin (internal code, or a loaded shared library).
 *
 * Conceptually, a Block is an instance of this.
 */
class PluginImpl : public Plugin
                 , public boost::noncopyable
{
public:
	PluginImpl(Ingen::URIs&     uris,
	           Type             type,
	           const Raul::URI& uri)
		: Plugin(uris, uri)
		, _type(type)
	{}

	virtual BlockImpl* instantiate(BufferFactory&      bufs,
	                               const Raul::Symbol& symbol,
	                               bool                polyphonic,
	                               GraphImpl*          parent,
	                               Engine&             engine) = 0;

	virtual const Raul::Symbol symbol() const = 0;

	Plugin::Type type() const         { return _type; }
	void         type(Plugin::Type t) { _type = t; }

protected:
	Plugin::Type _type;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_PLUGINIMPL_HPP
