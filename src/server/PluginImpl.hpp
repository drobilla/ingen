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

#ifndef INGEN_ENGINE_PLUGINIMPL_HPP
#define INGEN_ENGINE_PLUGINIMPL_HPP

#include "ingen/Resource.hpp"
#include "raul/Symbol.hpp"

#include <cstdlib>

namespace ingen {

class URIs;

namespace server {

class BlockImpl;
class BufferFactory;
class Engine;
class GraphImpl;

/** Implementation of a plugin (internal code, or a loaded shared library).
 *
 * Conceptually, a Block is an instance of this.
 */
class PluginImpl : public Resource
{
public:
	PluginImpl(ingen::URIs& uris, const Atom& type, const URI& uri)
	    : Resource(uris, uri)
	    , _type(type)
	    , _presets_loaded(false)
	    , _is_zombie(false)
	{
	}

	virtual BlockImpl* instantiate(BufferFactory&      bufs,
	                               const Raul::Symbol& symbol,
	                               bool                polyphonic,
	                               GraphImpl*          parent,
	                               Engine&             engine,
	                               const LilvState*    state) = 0;

	virtual const Raul::Symbol symbol() const = 0;

	const Atom& type() const            { return _type; }
	void        set_type(const Atom& t) { _type = t; }
	bool        is_zombie() const       { return _is_zombie; }
	void        set_is_zombie(bool t)   { _is_zombie = t; }

	typedef std::pair<URI, std::string> Preset;
	typedef std::map<URI, std::string>  Presets;

	const Presets& presets(bool force_reload=false) {
		if (!_presets_loaded || force_reload) {
			load_presets();
		}

		return _presets;
	}

	virtual void update_properties() {}

	virtual void load_presets() { _presets_loaded = true; }

	virtual URI bundle_uri() const { return URI("ingen:/"); }

protected:
	Atom    _type;
	Presets _presets;
	bool    _presets_loaded;
	bool    _is_zombie;

private:
	PluginImpl(const PluginImpl&) = delete;
	PluginImpl& operator=(const PluginImpl&) = delete;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_PLUGINIMPL_HPP
