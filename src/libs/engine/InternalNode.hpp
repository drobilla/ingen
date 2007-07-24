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

#ifndef INTERNALNODE_H
#define INTERNALNODE_H

#include <cstdlib>
#include "NodeBase.hpp"
#include "Plugin.hpp"
#include "types.hpp"

namespace Ingen {

class Patch;


/** Base class for Internal (builtin) nodes
 *
 * \ingroup engine
 */
class InternalNode : public NodeBase
{
public:
	InternalNode(const Plugin* plugin, const string& path, size_t poly, Patch* parent, SampleRate srate, size_t buffer_size)
	: NodeBase(plugin, path, poly, parent, srate, buffer_size)
	{
	}
	
	virtual ~InternalNode() {}

protected:
	Plugin* plugin() const { return const_cast<Plugin*>(_plugin); }
};


} // namespace Ingen

#endif // INTERNALNODE_H
