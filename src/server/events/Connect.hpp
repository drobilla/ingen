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

#ifndef INGEN_EVENTS_CONNECT_HPP
#define INGEN_EVENTS_CONNECT_HPP

#include "raul/Path.hpp"
#include "Event.hpp"
#include "PatchImpl.hpp"
#include "InputPort.hpp"
#include "types.hpp"

namespace Raul {
	template <typename T> class ListNode;
	template <typename T> class Array;
}

namespace Ingen {
namespace Server {

class PatchImpl;
class NodeImpl;
class ConnectionImpl;
class PortImpl;
class InputPort;
class OutputPort;
class CompiledPatch;

namespace Events {

/** Make a Connection between two Ports.
 *
 * \ingroup engine
 */
class Connect : public Event
{
public:
	Connect(Engine&           engine,
	        Interface*        client,
	        int32_t           id,
	        SampleCount       timestamp,
	        const Raul::Path& src_port_path,
	        const Raul::Path& dst_port_path);

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	Raul::Path _src_port_path;
	Raul::Path _dst_port_path;

	PatchImpl*  _patch;
	OutputPort* _src_output_port;
	InputPort*  _dst_input_port;

	CompiledPatch* _compiled_patch; ///< New process order for Patch

	SharedPtr<ConnectionImpl> _connection;

	Raul::Array<BufferFactory::Ref>* _buffers;
};

} // namespace Server
} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_CONNECT_HPP
