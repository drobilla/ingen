/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
#include "ConnectionModel.h"
#include "PortModel.h"
#include "PatchModel.h"

namespace Ingen {
namespace Client {


ConnectionModel::ConnectionModel(const Path& src_port, const Path& dst_port)
: _src_port_path(src_port),
  _dst_port_path(dst_port)
{
	// Be sure connection is within one patch
	//assert(_src_port_path.parent().parent()
	//	== _dst_port_path.parent().parent());
}


ConnectionModel::ConnectionModel(SharedPtr<PortModel> src, SharedPtr<PortModel> dst)
: _src_port_path(src->path()),
  _dst_port_path(dst->path()),
  _src_port(src),
  _dst_port(dst)
{
	assert(_src_port);
	assert(_dst_port);
	assert(_src_port->parent());
	assert(_dst_port->parent());

	// Be sure connection is within one patch
	//assert(_src_port_path.parent().parent()
	//	== _dst_port_path.parent().parent());
}


const Path&
ConnectionModel::src_port_path() const
{
	if (!_src_port)
		return _src_port_path;
	else
		return _src_port->path();
}


const Path&
ConnectionModel::dst_port_path() const
{
	if (!_dst_port)
		return _dst_port_path;
	else
		return _dst_port->path();
}

const Path
ConnectionModel::patch_path() const
{
	// Resolved
	if (_src_port && _dst_port) {
		// Direct connection from patch input to patch output (pass through)
		// (parent patch is parent of ports)
		if (_src_port->parent() == _dst_port->parent()) {
			SharedPtr<PatchModel> parent_patch = PtrCast<PatchModel>(_src_port->parent());
			if (parent_patch)
				return parent_patch->path();
		}
	}

	// Aside from the above special case, parent patch is parent of parent of ports
		
	const Path& src_node = _src_port_path.parent();
	const Path& dst_node = _dst_port_path.parent();
	Path patch_path = src_node.parent();

	if (src_node.parent() != dst_node.parent()) {
		// Connection to a patch port from inside the patch
		assert(src_node.parent() == dst_node || dst_node.parent() == src_node);
		if (src_node.parent() == dst_node)
			patch_path = dst_node;
		else
			patch_path = src_node;
	}

	return patch_path;
}

typedef list<SharedPtr<ConnectionModel> > ConnectionList;


} // namespace Client
} // namespace Ingen
