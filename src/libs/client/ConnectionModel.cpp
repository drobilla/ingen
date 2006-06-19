/* This file is part of Om.  Copyright (C) 2005 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ConnectionModel.h"
#include "PortModel.h"

namespace LibOmClient {


ConnectionModel::ConnectionModel(const Path& src_port, const Path& dst_port)
: m_src_port_path(src_port),
  m_dst_port_path(dst_port),
  m_src_port(NULL),
  m_dst_port(NULL)
{
	// Be sure connection is within one patch
	//assert(m_src_port_path.parent().parent()
	//	== m_dst_port_path.parent().parent());
}


const Path&
ConnectionModel::src_port_path() const
{
	if (m_src_port == NULL)
		return m_src_port_path;
	else
		return m_src_port->path();
}


const Path&
ConnectionModel::dst_port_path() const
{
	if (m_dst_port == NULL)
		return m_dst_port_path;
	else
		return m_dst_port->path();
}

const Path
ConnectionModel::patch_path() const
{
	const Path& src_node = m_src_port_path.parent();
	const Path& dst_node = m_dst_port_path.parent();
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


} // namespace LibOmClient
