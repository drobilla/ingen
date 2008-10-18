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

#ifndef LADSPAPLUGIN_H
#define LADSPAPLUGIN_H

#include "config.h"

#include <cstdlib>
#include <glibmm/module.h>
#include <boost/utility.hpp>
#include <dlfcn.h>
#include <string>
#include <iostream>
#include "raul/Path.hpp"
#include "types.hpp"
#include "PluginImpl.hpp"

namespace Ingen {

class NodeImpl;


/** Implementation of a LADSPA plugin (loaded shared library).
 */
class LADSPAPlugin : public PluginImpl
{
public:
	LADSPAPlugin(const std::string& library_path,
	             const std::string& uri,
	             unsigned long      id,
	             const string&      label,
	             const string&      name)
		: PluginImpl(Plugin::LADSPA, uri, library_path)
		, _id(id)
		, _label(label)
		, _name(name)
	{}
	
	NodeImpl* instantiate(const std::string& name,
	                      bool               polyphonic,
	                      Ingen::PatchImpl*  parent,
	                      Engine&            engine);
	
	const std::string& label()  const { return _label; }
	unsigned long      id()     const { return _id; }
	const string       symbol() const { return Raul::Path::nameify(_label); }
	const string       name()   const { return _name; }

	const string library_name() const {
		return _library_path.substr(_library_path.find_last_of("/")+1);
	}
	
private:
	const unsigned long _id;
	const std::string   _label;
	const std::string   _name;
};


} // namespace Ingen

#endif // LADSPAPLUGIN_H

