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

#ifndef CONNECTIONBASE_H
#define CONNECTIONBASE_H

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
class ConnectionBase : public Connection
{
public:
	ConnectionBase(OutputPort<T>* const src_port, InputPort<T>* const dst_port);
	virtual ~ConnectionBase();

	void prepare_buffers();

	inline OutputPort<T>* src_port() const { return (OutputPort<T>*)m_src_port; }
	inline InputPort<T>*  dst_port() const { return (InputPort<T>*)m_dst_port; }

	/** Used by some (recursive) events to prevent double disconnections */
	bool pending_disconnection()       { return m_pending_disconnection; }
	void pending_disconnection(bool b) { m_pending_disconnection = b; }
	
	/** Get the buffer for a particular voice.
	 * A ConnectionBase is smart - it knows the destination port respondering the
	 * buffer, and will return accordingly (ie the same buffer for every voice
	 * in a mono->poly connection).
	 */
	inline Buffer<T>* buffer(size_t voice) const;
	
private:
	// Disallow copies (undefined)
	ConnectionBase(const ConnectionBase& copy);
	ConnectionBase& operator=(const ConnectionBase&);

	Buffer<T>* m_local_buffer;  ///< Only used for poly->mono connections
	bool       m_is_poly_to_mono;
	size_t     m_buffer_size;
	bool       m_pending_disconnection;
};


template <>
inline Buffer<sample>* 
ConnectionBase<sample>::buffer(size_t voice) const
{
	PortBase<sample>* const src_port = (PortBase<sample>*)m_src_port;
	
	if (m_is_poly_to_mono) {
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
ConnectionBase<MidiMessage>::buffer(size_t voice) const
{
	// No such thing as polyphonic MIDI ports
	assert(m_src_port->poly() == 1);
	assert(m_dst_port->poly() == 1);

	PortBase<MidiMessage>* const src_port = (PortBase<MidiMessage>*)m_src_port;
	return src_port->buffer(0);
}


template class ConnectionBase<sample>;
template class ConnectionBase<MidiMessage>;

} // namespace Om

#endif // CONNECTIONBASE_H
