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

#ifndef PATCHPLUGIN_H
#define PATCHPLUGIN_H

#include CONFIG_H_PATH

#include <string>
#include "PluginImpl.hpp"

namespace Ingen {

class NodeImpl;


/** Implementation of a Patch plugin.
 *
 * Patches don't actually work like this yet...
 */
class PatchPlugin : public PluginImpl
{
public:
	PatchPlugin(const std::string& uri,
	            const std::string& symbol,
	            const std::string& name)
		: PluginImpl(Plugin::Patch, uri)
	{}
	
	NodeImpl* instantiate(const std::string& name,
	                      bool               polyphonic,
	                      Ingen::PatchImpl*  parent,
	                      SampleRate         srate,
	                      size_t             buffer_size)
	{
		return NULL;
	}
	
	const string symbol() const { return "patch"; }
	const string name()   const { return "Ingen Patch"; }

private:
	const string _symbol;
	const string _name;
};


} // namespace Ingen

#endif // PATCHPLUGIN_H

