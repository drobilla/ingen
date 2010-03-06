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

#ifndef INGEN_ENGINE_INTERNALPLUGIN_HPP
#define INGEN_ENGINE_INTERNALPLUGIN_HPP

#include "ingen-config.h"

#include <cstdlib>
#include <glibmm/module.h>
#include <boost/utility.hpp>
#include <dlfcn.h>
#include <string>
#include "PluginImpl.hpp"

#define NS_INTERNALS "http://drobilla.net/ns/ingen-internals#"

namespace Ingen {

class NodeImpl;
class BufferFactory;


/** Implementation of an Internal plugin.
 */
class InternalPlugin : public PluginImpl
{
public:
	InternalPlugin(Shared::LV2URIMap& uris,
			const std::string& uri, const std::string& symbol);

	NodeImpl* instantiate(BufferFactory&     bufs,
	                      const std::string& name,
	                      bool               polyphonic,
	                      Ingen::PatchImpl*  parent,
	                      Engine&            engine);

	const std::string symbol() const { return _symbol; }

private:
	const std::string _symbol;
};


} // namespace Ingen

#endif // INGEN_ENGINE_INTERNALPLUGIN_HPP

