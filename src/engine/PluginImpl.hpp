/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef INGEN_ENGINE_PLUGINIMPL_HPP
#define INGEN_ENGINE_PLUGINIMPL_HPP

#include <cstdlib>
#include <glibmm/module.h>
#include <boost/utility.hpp>
#include <dlfcn.h>
#include <string>
#include "interface/Plugin.hpp"
#include "shared/ResourceImpl.hpp"

namespace Ingen {

namespace Shared { class LV2URIMap; }

class PatchImpl;
class NodeImpl;
class Engine;
class BufferFactory;


/** Implementation of a plugin (internal code, or a loaded shared library).
 *
 * Conceptually, a Node is an instance of this.
 */
class PluginImpl : public Ingen::Shared::Plugin
                 , public Ingen::Shared::ResourceImpl
                 , public boost::noncopyable
{
public:
	PluginImpl(Shared::LV2URIMap& uris,
			Type type, const std::string& uri, const std::string library_path="")
		: ResourceImpl(uris, uri)
		, _type(type)
		, _library_path(library_path)
		, _module(NULL)
	{}

	virtual NodeImpl* instantiate(BufferFactory&     bufs,
	                              const std::string& name,
	                              bool               polyphonic,
	                              Ingen::PatchImpl*  parent,
	                              Engine&            engine) = 0;

	virtual const std::string symbol() const = 0;

	virtual const std::string& library_path() const { return _library_path; }

	void load();
	void unload();

	Plugin::Type  type() const                 { return _type; }
	void          type(Plugin::Type t)         { _type = t; }
	Glib::Module* module() const               { return _module; }
	void          module(Glib::Module* module) { _module = module; }

protected:
	Plugin::Type        _type;
	mutable std::string _library_path;
	Glib::Module*       _module;
};


} // namespace Ingen

#endif // INGEN_ENGINE_PLUGINIMPL_HPP

