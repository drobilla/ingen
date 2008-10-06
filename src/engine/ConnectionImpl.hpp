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

#ifndef CONNECTIONIMPL_H
#define CONNECTIONIMPL_H

#include <cstdlib>
#include <boost/utility.hpp>
#include <raul/Deletable.hpp>
#include "interface/DataType.hpp"
#include "interface/Connection.hpp"
#include "PortImpl.hpp"
#include "types.hpp"

namespace Ingen {

class PortImpl;
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
class ConnectionImpl : public Raul::Deletable, public Shared::Connection
{
public:
	ConnectionImpl(PortImpl* src_port, PortImpl* dst_port);
	virtual ~ConnectionImpl();
	
	PortImpl* src_port() const { return _src_port; }
	PortImpl* dst_port() const { return _dst_port; }
	
	const Raul::Path src_port_path() const { return _src_port->path(); }
	const Raul::Path dst_port_path() const { return _dst_port->path(); }

	/** Used by some (recursive) events to prevent double disconnections */
	bool pending_disconnection()       { return _pending_disconnection; }
	void pending_disconnection(bool b) { _pending_disconnection = b; }
	
	void process(ProcessContext& context);

	/** Get the buffer for a particular voice.
	 * A Connection is smart - it knows the destination port requesting the
	 * buffer, and will return accordingly (ie the same buffer for every voice
	 * in a mono->poly connection).
	 */
	inline Buffer* buffer(size_t voice) const;

	inline size_t buffer_size() const { return _buffer_size; }
	
	void set_buffer_size(size_t size);
	void prepare_poly(uint32_t poly);
	void apply_poly(Raul::Maid& maid, uint32_t poly);

	bool must_mix() const;
	bool must_extend() const;

	inline bool need_buffer() const { return must_mix(); }
	inline bool can_direct() const { return _mode == DIRECT; }

	DataType type() const { return _src_port->type(); }

protected:
	enum { DIRECT, MIX, EXTEND } _mode;
	void set_mode();

	PortImpl* const _src_port;
	PortImpl* const _dst_port;
	Buffer*         _local_buffer;
	size_t          _buffer_size;
	bool            _pending_disconnection;
};


inline Buffer*
ConnectionImpl::buffer(size_t voice) const
{
	if (_mode == MIX) {
		return _local_buffer;
	} else if ( ! _src_port->polyphonic()) {
		return _src_port->buffer(0);
	} else {
		return _src_port->buffer(voice);
	}
}


} // namespace Ingen

#endif // CONNECTIONIMPL_H
