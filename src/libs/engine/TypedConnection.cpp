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

#include "TypedConnection.h"
#include "util.h"
#include "InputPort.h"
#include "OutputPort.h"
#include "Node.h"
#include "Om.h"
#include "Port.h"

namespace Om {


/** Constructor for a connection from a node's output port.
 *
 * This handles both polyphonic and monophonic nodes, transparently to the 
 * user (InputPort).
 */
template <typename T>
TypedConnection<T>::TypedConnection(OutputPort<T>* const src_port, InputPort<T>* const dst_port)
: Connection(src_port, dst_port),
  m_local_buffer(NULL),
  m_is_poly_to_mono( (src_port->parent_node()->poly() > dst_port->parent_node()->poly()) ),
  m_buffer_size(src_port->buffer_size()),
  m_pending_disconnection(false)
{
	assert((src_port->parent_node()->poly() == dst_port->parent_node()->poly())
		|| (src_port->parent_node()->poly() == 1 || dst_port->parent_node()->poly() == 1));

	if (m_is_poly_to_mono) // Poly -> Mono connection, need a buffer to mix in to
		m_local_buffer = new Buffer<T>(m_buffer_size);
}
template TypedConnection<sample>::TypedConnection(OutputPort<sample>* const src_port, InputPort<sample>* const dst_port);
template TypedConnection<MidiMessage>::TypedConnection(OutputPort<MidiMessage>* const src_port, InputPort<MidiMessage>* const dst_port);


template <typename T>
TypedConnection<T>::~TypedConnection()
{
	delete m_local_buffer;
}
template TypedConnection<sample>::~TypedConnection();
template TypedConnection<MidiMessage>::~TypedConnection();


template <typename sample>
void
TypedConnection<sample>::process(samplecount nframes)
{
	// FIXME: nframes parameter not used
	assert(nframes == m_buffer_size);

	/* Thought:  A poly output port can be connected to multiple mono input
	 * ports, which means this mix down would have to happen many times.
	 * Adding a method to OutputPort that mixes down all it's outputs into
	 * a buffer (if it hasn't been done already this cycle) and returns that
	 * would avoid having to mix multiple times.  Probably not a very common
	 * case, but it would be faster anyway. */
	
	if (m_is_poly_to_mono) {
		m_local_buffer->copy(src_port()->buffer(0), 0, m_buffer_size-1);
	
		// Mix all the source's voices down into local buffer starting at the second
		// voice (buffer is already set to first voice above)
		for (size_t j=1; j < src_port()->poly(); ++j)
			m_local_buffer->accumulate(src_port()->buffer(j), 0, m_buffer_size-1);

		// Scale the buffer down.
		if (src_port()->poly() > 1)
			m_local_buffer->scale(1.0f/(float)src_port()->poly(), 0, m_buffer_size-1);
	}
}
template void TypedConnection<sample>::process(samplecount nframes);


// FIXME: MIDI mixing not implemented
template <>
void
TypedConnection<MidiMessage>::process(samplecount nframes)
{
}


} // namespace Om

