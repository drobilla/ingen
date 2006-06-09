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

#include "OutputPort.h"
#include "InputPort.h"
#include "PortInfo.h"
#include <cassert>

namespace Om {
		
template<typename T>
OutputPort<T>::OutputPort(Node* node, const string& name, size_t index, size_t poly, PortInfo* port_info, size_t buffer_size)
: PortBase<T>(node, name, index, poly, port_info, buffer_size)
{
	assert(port_info->is_output() && !port_info->is_input());
}
template OutputPort<sample>::OutputPort(Node* node, const string& name, size_t index, size_t poly, PortInfo* port_info, size_t buffer_size);
template OutputPort<MidiMessage>::OutputPort(Node* node, const string& name, size_t index, size_t poly, PortInfo* port_info, size_t buffer_size);


template<typename T>
void
OutputPort<T>::set_tied_port(InputPort<T>* port)
{
	assert(!m_is_tied);
	assert(m_tied_port == NULL);
	assert(static_cast<PortBase<T>*>(port) != static_cast<PortBase<T>*>(this));
	assert(port != NULL);

	m_is_tied = true;
	m_tied_port = (PortBase<T>*)port;
}
template void OutputPort<sample>::set_tied_port(InputPort<sample>* port);
template void OutputPort<MidiMessage>::set_tied_port(InputPort<MidiMessage>* port);


} // namespace Om

