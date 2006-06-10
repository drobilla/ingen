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

#include "ControlInputNode.h"
#include "util/types.h"
#include "Patch.h"
#include "OutputPort.h"
#include "InputPort.h"
#include "Plugin.h"
#include "PortInfo.h"

namespace Om {


ControlInputNode::ControlInputNode(const string& path, size_t poly, Patch* parent, samplerate srate, size_t buffer_size)
: BridgeNode<sample>(path, poly, parent, srate, buffer_size)
{
	OutputPort<sample>* internal_port = new OutputPort<sample>(this, "in", 0, m_poly,
		new PortInfo("in", CONTROL, OUTPUT), 1);
	InputPort<sample>* external_port = new InputPort<sample>(parent, m_name, 0, m_poly,
		new PortInfo(m_name, CONTROL, INPUT), 1);
	external_port->tie(internal_port);
	m_external_port = external_port;

	m_num_ports = 1;
	m_ports.alloc(m_num_ports);
	m_ports.at(0) = internal_port;
	
	m_plugin.type(Plugin::Internal);
	m_plugin.plug_label("control_input");
	m_plugin.name("Om Patch Control Input Node");
}


} // namespace Om

