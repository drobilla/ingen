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

#include <sstream>
#include <raul/Path.h>
#include "PluginModel.h"
#include "PatchModel.h"


namespace Ingen {
namespace Client {

SLV2World   PluginModel::_slv2_world   = NULL;
SLV2Plugins PluginModel::_slv2_plugins = NULL;
	

string
PluginModel::default_node_name(SharedPtr<PatchModel> parent)
{
	string default_name = Raul::Path::nameify(_name);
	string name;

	char num_buf[3];
	for (uint i=0; i < 99; ++i) {
		name = default_name;
		if (i != 0) {
			snprintf(num_buf, 3, "%d", i+1);
			name += num_buf;
		}
		if (!parent->get_node(name))
			break;
	}

	return name;
}


} // namespace Client
} // namespace Ingen
