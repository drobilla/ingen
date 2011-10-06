/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include <boost/intrusive/list.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/utility.hpp>

#include "ingen/Connection.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "raul/Deletable.hpp"
#include "raul/log.hpp"

#include "BufferFactory.hpp"
#include "Context.hpp"

using namespace std;

namespace Ingen {
namespace Server {

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
class ConnectionImpl : public  Raul::Deletable
                     , private Raul::Noncopyable
                     , public  Connection
                     , public  boost::intrusive::list_base_hook<>
{
public:
	ConnectionImpl(PortImpl* src_port, PortImpl* dst_port);

	PortImpl* src_port() const { return _src_port; }
	PortImpl* dst_port() const { return _dst_port; }

	const Raul::Path& src_port_path() const;
	const Raul::Path& dst_port_path() const;

	/** Used by some (recursive) events to prevent double disconnections */
	bool pending_disconnection()       { return _pending_disconnection; }
	void pending_disconnection(bool b) { _pending_disconnection = b; }

	void queue(Context& context);

	void get_sources(Context&                      context,
	                 uint32_t                      voice,
	                 boost::intrusive_ptr<Buffer>* srcs,
	                 uint32_t                      max_num_srcs,
	                 uint32_t&                     num_srcs);

	/** Get the buffer for a particular voice.
	 * A Connection is smart - it knows the destination port requesting the
	 * buffer, and will return accordingly (e.g. the same buffer for every
	 * voice in a mono->poly connection).
	 */
	BufferFactory::Ref buffer(uint32_t voice) const;

	/** Returns true if this connection must mix down voices into a local buffer */
	bool must_mix() const;

	/** Returns true if this connection crosses contexts and must buffer */
	bool must_queue() const;

	static bool can_connect(const OutputPort* src, const InputPort* dst);

protected:
	void dump() const;

	PortImpl* const   _src_port;
	PortImpl* const   _dst_port;
	Raul::RingBuffer* _queue;
	bool              _pending_disconnection;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_CONNECTIONIMPL_HPP
