/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef INTERNALPLUGIN_H
#define INTERNALPLUGIN_H

#include "config.h"

#ifndef HAVE_SLV2
#error "This file requires SLV2, but HAVE_SLV2 is not defined.  Please report."
#endif

#include <cstdlib>
#include <glibmm/module.h>
#include <boost/utility.hpp>
#include <dlfcn.h>
#include <string>
#include <iostream>
#include "slv2/slv2.h"
#include "types.hpp"
#include "PluginImpl.hpp"

#define NS_INGEN "http://drobilla.net/ns/ingen#"

namespace Ingen {

class NodeImpl;


/** Implementation of an Internal plugin.
 */
class InternalPlugin : public PluginImpl
{
public:
	InternalPlugin(const std::string& uri,
	               const std::string& symbol,
	               const std::string& name)
		: PluginImpl(Plugin::Internal, uri)
		, _symbol(symbol)
		, _name(name)
	{}
	
	NodeImpl* instantiate(const std::string& name,
	                      bool               polyphonic,
	                      Ingen::PatchImpl*  parent,
	                      Engine&            engine);
	
	const string symbol() const { return _symbol; }
	const string name()   const { return _name; }

private:
	const string _symbol;
	const string _name;
};


} // namespace Ingen

#endif // INTERNALPLUGIN_H

