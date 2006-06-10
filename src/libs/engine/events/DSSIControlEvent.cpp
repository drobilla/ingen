/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
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

#include "DSSIControlEvent.h"
#include "Om.h"
#include "OmApp.h"
#include "Node.h"
#include "Plugin.h"
#include "ObjectStore.h"

namespace Om {


DSSIControlEvent::DSSIControlEvent(CountedPtr<Responder> responder, const string& node_path, int port_num, sample val)
: QueuedEvent(responder),
  m_node_path(node_path),
  m_port_num(port_num),
  m_val(val),
  m_node(NULL)
{
}


void
DSSIControlEvent::pre_process()
{
	Node* node = om->object_store()->find_node(m_node_path);

	if (node->plugin()->type() != Plugin::DSSI)
		m_node = NULL;
	else
		m_node = (DSSIPlugin*)node;

	QueuedEvent::pre_process();
}

	
void
DSSIControlEvent::execute(samplecount offset)
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


} // namespace Om

