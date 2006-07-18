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

#ifndef TYPEDCONNECTION_H
#define TYPEDCONNECTION_H

#include "types.h"
#include "OutputPort.h"
#include "Connection.h"

namespace Om {

class MidiMessage;
class Port;
template <typename T> class InputPort;


/** A Connection with a type.
 *
 * \ingroup engine
 */
template <typename T>
class TypedConnection : public Connection
{
public:
	TypedConnection(OutputPort<T>* const src_port, InputPort<T>* const dst_port);
	virtual ~TypedConnection();

	void process(SampleCount nframes);

	inline OutputPort<T>* src_port() const { return dynamic_cast<OutputPort<T>*>(m_src_port); }
	inline InputPort<T>*  dst_port() const { return dynamic_cast<InputPort<T>*>(m_dst_port); }

	/** Used by some (recursive) events to prevent double disconnections */
	bool pending_disconnection()       { return m_pending_disconnection; }
	void pending_disconnection(bool b) { m_pending_disconnection = b; }
	
	/** Get the buffer for a particular voice.
	 * A TypedConnection is smart - it knows the destination port respondering the
	 * buffer, and will return accordingly (ie the same buffer for every voice
	 * in a mono->poly connection).
	 */
	inline Buffer<T>* buffer(size_t voice) const;
	
private:
	// Disallow copies (undefined)
	TypedConnection(const TypedConnection& copy);
	TypedConnection& operator=(const TypedConnection&);

	Buffer<T>* m_local_buffer;  ///< Only used for poly->mono connections
	bool       m_must_mix;
	size_t     m_buffer_size;
	bool       m_pending_disconnection;
};


template <>
inline Buffer<Sample>* 
TypedConnection<Sample>::buffer(size_t voice) const
{
	TypedPort<Sample>* const src_port = (TypedPort<Sample>*)m_src_port;
	
	if (m_must_mix) {
		return m_local_buffer;
	} else {
		if (src_port->poly() == 1)
			return src_port->buffer(0);
		else
			return src_port->buffer(voice);
	}
}


template <>
inline Buffer<MidiMessage>*
TypedConnection<MidiMessage>::buffer(size_t voice) const
{
	// No such thing as polyphonic MIDI ports
	assert(m_src_port->poly() == 1);
	assert(m_dst_port->poly() == 1);

	TypedPort<MidiMessage>* const src_port = (TypedPort<MidiMessage>*)m_src_port;
	return src_port->buffer(0);
}


template class TypedConnection<Sample>;
template class TypedConnection<MidiMessage>;

} // namespace Om

#endif // TYPEDCONNECTION_H
