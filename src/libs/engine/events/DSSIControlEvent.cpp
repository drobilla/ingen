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

#include "DSSIControlEvent.h"
#include "Ingen.h"
#include "Node.h"
#include "Plugin.h"
#include "ObjectStore.h"

namespace Ingen {


DSSIControlEvent::DSSIControlEvent(CountedPtr<Responder> responder, SampleCount timestamp, const string& node_path, int port_num, Sample val)
: QueuedEvent(responder, timestamp),
  m_node_path(node_path),
  m_port_num(port_num),
  m_val(val),
  m_node(NULL)
{
}


void
DSSIControlEvent::pre_process()
{
	Node* node = Ingen::instance().object_store()->find_node(m_node_path);

	if (node->plugin()->type() != Plugin::DSSI)
		m_node = NULL;
	else
		m_node = (DSSINode*)node;

	QueuedEvent::pre_process();
}

	
void
DSSIControlEvent::execute(SampleCount offset)
{
	if (m_node != NULL)
		m_node->set_control(m_port_num, m_val);
}


void
DSSIControlEvent::post_process()
{
	if (m_node == NULL)
		std::cerr << "Unable to find DSSI node " << m_node_path << std::endl;
}


} // namespace Ingen

