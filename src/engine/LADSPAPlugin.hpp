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

#ifndef INGEN_ENGINE_LADSPAPLUGIN_HPP
#define INGEN_ENGINE_LADSPAPLUGIN_HPP

#include <cstdlib>
#include <glibmm/module.h>
#include <boost/utility.hpp>
#include <dlfcn.h>
#include <string>
#include "raul/Path.hpp"
#include "raul/Atom.hpp"
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
	             const std::string& label,
	             const std::string& name);

	NodeImpl* instantiate(BufferFactory& bufs,
	                      const std::string& name,
	                      bool               polyphonic,
	                      Ingen::PatchImpl*  parent,
	                      Engine&            engine);

	const std::string& label()  const { return _label; }
	unsigned long      id()     const { return _id; }
	const std::string  symbol() const { return Raul::Path::nameify(_label); }
	const std::string  name()   const { return _name.get_string(); }

	const std::string library_name() const {
		return _library_path.substr(_library_path.find_last_of("/")+1);
	}

	const Raul::Atom& get_property(const Raul::URI& uri) const;

private:
	const unsigned long _id;
	const std::string   _label;
	const Raul::Atom    _name;
};


} // namespace Ingen

#endif // INGEN_ENGINE_LADSPAPLUGIN_HPP

