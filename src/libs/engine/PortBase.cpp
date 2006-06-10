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

#include "PortBase.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <sys/mman.h>
#include "util.h"
#include "Node.h"
#include "PortInfo.h"
#include "MidiMessage.h"

namespace Om {


/** Constructor for a Port.
 */
template <typename T>
PortBase<T>::PortBase(Node* node, const string& name, size_t index, size_t poly, PortInfo* port_info, size_t buffer_size)
: Port(node, name, index, poly, port_info),
  m_buffer_size(buffer_size),
  m_fixed_buffers(false),
  m_is_tied(false),
  m_tied_port(NULL)
{
	allocate_buffers();
	clear_buffers();

	assert(m_buffers.size() > 0);
}
template
PortBase<sample>::PortBase(Node* node, const string& name, size_t index, size_t poly, PortInfo* port_info, size_t buffer_size);
template
PortBase<MidiMessage>::PortBase(Node* node, const string& name, size_t index, size_t poly, PortInfo* port_info, size_t buffer_size);


template <typename T>
PortBase<T>::~PortBase()
{
	for (size_t i=0; i < m_poly; ++i)
		delete m_buffers.at(i);

	delete m_port_info;
}
template PortBase<sample>::~PortBase();
template PortBase<MidiMessage>::~PortBase();


/** Set the port's value for all voices.
 */
template<>
void
PortBase<sample>::set_value(sample val, size_t offset)
{
	if (m_port_info->is_control())
		offset = 0;
	assert(offset < m_buffer_size);

	for (size_t v=0; v < m_poly; ++v)
		m_buffers.at(v)->set(val, offset);
}

/** Set the port's value for a specific voice.
 */
template<>
void
PortBase<sample>::set_value(size_t voice, sample val, size_t offset)
{
	if (m_port_info->is_control())
		offset = 0;
	assert(offset < m_buffer_size);

	m_buffers.at(voice)->set(val, offset);
}


template <typename T>
void
PortBase<T>::allocate_buffers()
{
	m_buffers.alloc(m_poly);

	for (size_t i=0; i < m_poly; ++i)
		m_buffers.at(i) = new Buffer<T>(m_buffer_size);
}
template void PortBase<sample>::allocate_buffers();
template void PortBase<MidiMessage>::allocate_buffers();

	
template<>
void
PortBase<sample>::prepare_buffers(size_t nframes)
{
	for (size_t i=0; i < m_poly; ++i)
		m_buffers.at(i)->prepare(nframes);
}


template<>
void
PortBase<MidiMessage>::prepare_buffers(size_t nframes)
{
}


template<typename T>
void
PortBase<T>::clear_buffers()
{
	for (size_t i=0; i < m_poly; ++i)
		m_buffers.at(i)->clear();
}
template void PortBase<sample>::clear_buffers();
template void PortBase<MidiMessage>::clear_buffers();


} // namespace Om

