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

#include <cassert>
#include "LV2Plugin.hpp"
#include "LV2Node.hpp"
#include "NodeImpl.hpp"

namespace Ingen {

	
const string
LV2Plugin::symbol() const
{
	string working = _uri;
	if (working[working.length()-1] == '/')
		working = working.substr(0, working.length()-1);

	while (working.length() > 0) {
		size_t last_slash = working.find_last_of("/");
		const string symbol = working.substr(last_slash+1);
		if ( (symbol[0] >= 'a' && symbol[0] <= 'z')
				|| (symbol[0] >= 'A' && symbol[0] <= 'Z') )
			return Path::nameify(symbol);
		else
			working = working.substr(0, last_slash);
	}
	
	return "lv2_symbol";
}


const string
LV2Plugin::name() const
{
	SLV2Value name = slv2_plugin_get_name(_slv2_plugin);
	if (name)
		return slv2_value_as_string(name);
	else
		return "(no name)";
}


NodeImpl*
LV2Plugin::instantiate(const string&     name,
                       bool              polyphonic,
                       Ingen::PatchImpl* parent,
                       SampleRate        srate,
                       size_t            buffer_size)
{
	load(); // FIXME: unload at some point
	
	LV2Node* n = new LV2Node(this, name, polyphonic, parent, srate, buffer_size);

	if ( ! n->instantiate() ) {
		delete n;
		n = NULL;
	}

	return n;
}


} // namespace Ingen
