/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_CONNECTIONIMPL_HPP
#define INGEN_ENGINE_CONNECTIONIMPL_HPP

#include <cstdlib>
#include <boost/utility.hpp>
#include "raul/log.hpp"
#include "raul/Deletable.hpp"
#include "raul/IntrusivePtr.hpp"
#include "interface/PortType.hpp"
#include "interface/Connection.hpp"
#include "atom.lv2/atom.h"
#include "PortImpl.hpp"
#include "PortImpl.hpp"

using namespace std;

namespace Ingen {

class PortImpl;
class OutputPort;
class InputPort;
class Buffer;
class BufferFactory;


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
	ConnectionImpl(BufferFactory& bufs, PortImpl* src_port, PortImpl* dst_port);

	PortImpl* src_port() const { return _src_port; }
	PortImpl* dst_port() const { return _dst_port; }

	const Raul::Path src_port_path() const { return _src_port->path(); }
	const Raul::Path dst_port_path() const { return _dst_port->path(); }

	/** Used by some (recursive) events to prevent double disconnections */
	bool pending_disconnection()       { return _pending_disconnection; }
	void pending_disconnection(bool b) { _pending_disconnection = b; }

	void queue(Context& context);

	void get_sources(Context& context, uint32_t voice,
			IntrusivePtr<Buffer>* srcs, uint32_t max_num_srcs, uint32_t& num_srcs);

	/** Get the buffer for a particular voice.
	 * A Connection is smart - it knows the destination port requesting the
	 * buffer, and will return accordingly (e.g. the same buffer for every
	 * voice in a mono->poly connection).
	 */
	inline BufferFactory::Ref buffer(uint32_t voice) const {
		assert(!must_mix());
		assert(!must_queue());
		assert(_src_port->poly() == 1 || _src_port->poly() > voice);
		if (_src_port->poly() == 1) {
			return _src_port->buffer(0);
		} else {
			return _src_port->buffer(voice);
		}
	}

	/** Returns true if this connection must mix down voices into a local buffer */
	inline bool must_mix() const { return _src_port->poly() > _dst_port->poly(); }

	/** Returns true if this connection crosses contexts and must buffer */
	inline bool must_queue() const { return _src_port->context() != _dst_port->context(); }

	static bool can_connect(const OutputPort* src, const InputPort* dst);

protected:
	void dump() const;

	Raul::RingBuffer<LV2_Atom>* _queue;

	BufferFactory&     _bufs;
	PortImpl* const    _src_port;
	PortImpl* const    _dst_port;
	bool               _pending_disconnection;
};


} // namespace Ingen

#endif // INGEN_ENGINE_CONNECTIONIMPL_HPP
