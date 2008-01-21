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

#ifndef LV2PLUGIN_H
#define LV2PLUGIN_H

#include CONFIG_H_PATH

#ifndef HAVE_SLV2
#error "This file requires SLV2, but HAVE_SLV2 is not defined.  Please report."
#endif

#include <cstdlib>
#include <glibmm/module.h>
#include <boost/utility.hpp>
#include <dlfcn.h>
#include <string>
#include <iostream>
#include <slv2/slv2.h>
#include <raul/SharedPtr.hpp>
#include "types.hpp"
#include "PluginImpl.hpp"
#include "LV2Info.hpp"

using std::string;
using Ingen::Shared::Plugin;


namespace Ingen {

class PatchImpl;
class NodeImpl;


/** Implementation of an LV2 plugin (loaded shared library).
 */
class LV2Plugin : public PluginImpl
{
public:
	LV2Plugin(SharedPtr<LV2Info> lv2_info, const string& uri)
		: PluginImpl(Plugin::LV2, uri)
		, _lv2_info(lv2_info)
	{}
	
	NodeImpl* instantiate(const string&     name,
	                      bool              polyphonic,
	                      Ingen::PatchImpl* parent,
	                      SampleRate        srate,
	                      size_t            buffer_size);
	
	const string symbol() const;
	const string name() const;

	SharedPtr<LV2Info> lv2_info() const { return _lv2_info; }

	SLV2Plugin slv2_plugin() const       { return _slv2_plugin; }
	void       slv2_plugin(SLV2Plugin p) { _slv2_plugin = p; }

private:
	SLV2Plugin         _slv2_plugin;
	SharedPtr<LV2Info> _lv2_info;
};


} // namespace Ingen

#endif // LV2PLUGIN_H

