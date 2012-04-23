/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
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

	Raul::Array<BufferRef>* _buffers;
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_CONNECT_HPP
