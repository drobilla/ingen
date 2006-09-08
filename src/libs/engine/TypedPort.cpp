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

#include "TypedPort.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <sys/mman.h>
#include "util.h"
#include "Node.h"
#include "MidiMessage.h"

namespace Ingen {


/** Constructor for a Port.
 */
template <typename T>
TypedPort<T>::TypedPort(Node* parent, const string& name, size_t index, size_t poly, DataType type, size_t buffer_size)
: Port(parent, name, index, poly, type, buffer_size)
, m_fixed_buffers(false)
{
	allocate_buffers();
	clear_buffers();

	assert(m_buffers.size() > 0);
}
template
TypedPort<Sample>::TypedPort(Node* parent, const string& name, size_t index, size_t poly, DataType type, size_t buffer_size);
template
TypedPort<MidiMessage>::TypedPort(Node* parent, const string& name, size_t index, size_t poly, DataType type, size_t buffer_size);


template <typename T>
TypedPort<T>::~TypedPort()
{
	for (size_t i=0; i < _poly; ++i)
		delete m_buffers.at(i);
}
template TypedPort<Sample>::~TypedPort();
template TypedPort<MidiMessage>::~TypedPort();


/** Set the port's value for all voices.
 */
template<>
void
TypedPort<Sample>::set_value(Sample val, size_t offset)
{
	if (offset >= _buffer_size)
		offset = 0;
	assert(offset < _buffer_size);
	
	cerr << path() << " setting value " << val << endl;

	for (size_t v=0; v < _poly; ++v)
		m_buffers.at(v)->set(val, offset);
}

/** Set the port's value for a specific voice.
 */
template<>
void
TypedPort<Sample>::set_value(size_t voice, Sample val, size_t offset)
{
	if (offset >= _buffer_size)
		offset = 0;
	assert(offset < _buffer_size);
	
	cerr << path() << " setting voice value " << val << endl;

	m_buffers.at(voice)->set(val, offset);
}


template <typename T>
void
TypedPort<T>::allocate_buffers()
{
	m_buffers.alloc(_poly);

	for (size_t i=0; i < _poly; ++i)
		m_buffers.at(i) = new Buffer<T>(_buffer_size);
}
template void TypedPort<Sample>::allocate_buffers();
template void TypedPort<MidiMessage>::allocate_buffers();


template <typename T>
void
TypedPort<T>::process(SampleCount nframes, FrameTime start, FrameTime end)
{
	for (size_t i=0; i < _poly; ++i)
		m_buffers.at(i)->prepare(nframes);
}


template<typename T>
void
TypedPort<T>::clear_buffers()
{
	for (size_t i=0; i < _poly; ++i)
		m_buffers.at(i)->clear();
}
template void TypedPort<Sample>::clear_buffers();
template void TypedPort<MidiMessage>::clear_buffers();


} // namespace Ingen

