/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "OutputPort.h"
#include "InputPort.h"
#include <cassert>

namespace Om {
		
template<typename T>
OutputPort<T>::OutputPort(Node* parent, const string& name, size_t index, size_t poly, DataType type, size_t buffer_size)
: TypedPort<T>(parent, name, index, poly, type, buffer_size)
{
}
template OutputPort<sample>::OutputPort(Node* parent, const string& name, size_t index, size_t poly, DataType type, size_t buffer_size);
template OutputPort<MidiMessage>::OutputPort(Node* parent, const string& name, size_t index, size_t poly, DataType type, size_t buffer_size);

#if 0
template<typename T>
void
OutputPort<T>::set_tied_port(InputPort<T>* port)
{
	//assert(!m_is_tied);
	//assert(m_tied_port == NULL);
	assert(static_cast<TypedPort<T>*>(port) != static_cast<TypedPort<T>*>(this));
	assert(port != NULL);

	//m_is_tied = true;
	//m_tied_port = (TypedPort<T>*)port;
}
template void OutputPort<sample>::set_tied_port(InputPort<sample>* port);
template void OutputPort<MidiMessage>::set_tied_port(InputPort<MidiMessage>* port);
#endif

} // namespace Om

