/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <cstdlib>
#include <boost/utility.hpp>
#include <raul/Deletable.h>
#include "DataType.h"
#include "Port.h"
#include "types.h"

namespace Ingen {

class Port;
class Buffer;


/** Represents a single inbound connection for an InputPort.
 *
 * This can be a group of ports (ie coming from a polyphonic Node) or
 * a single Port.  This class exists basically as an abstraction of mixing
 * down polyphonic inputs, so InputPort can just deal with mixing down
 * multiple connections (oblivious to the polyphonic situation of the
 * connection itself).
 *
 * \ingroup engine
 */
class Connection : public Raul::Deletable
{
public:
	Connection(Port* src_port, Port* dst_port);
	virtual ~Connection();
	
	Port* src_port() const { return _src_port; }
	Port* dst_port() const { return _dst_port; }

	/** Used by some (recursive) events to prevent double disconnections */
	bool pending_disconnection()       { return _pending_disconnection; }
	void pending_disconnection(bool b) { _pending_disconnection = b; }
	
	void process(SampleCount nframes, FrameTime start, FrameTime end);

	/** Get the buffer for a particular voice.
	 * A Connection is smart - it knows the destination port requesting the
	 * buffer, and will return accordingly (ie the same buffer for every voice
	 * in a mono->poly connection).
	 */
	inline Buffer* buffer(size_t voice) const;
	
	void set_buffer_size(size_t size);

	DataType type() const { return _src_port->type(); }

protected:
	Port* const _src_port;
	Port* const _dst_port;
	Buffer*     _local_buffer;
	size_t      _buffer_size;
	bool        _must_mix;
	bool        _pending_disconnection;
};


inline Buffer*
Connection::buffer(size_t voice) const
{
	if (_must_mix)
		return _local_buffer;
	else if (_src_port->poly() == 1)
		return _src_port->buffer(0);
	else
		return _src_port->buffer(voice);
}


} // namespace Ingen

#endif // CONNECTION_H
