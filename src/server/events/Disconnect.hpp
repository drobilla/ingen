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

#ifndef INGEN_EVENTS_DISCONNECT_HPP
#define INGEN_EVENTS_DISCONNECT_HPP

#include "raul/Path.hpp"

#include "BufferFactory.hpp"
#include "Event.hpp"
#include "GraphImpl.hpp"
#include "types.hpp"

namespace Raul {
template <typename T> class Array;
}

namespace Ingen {
namespace Server {

class CompiledGraph;
class InputPort;
class OutputPort;
class PortImpl;

namespace Events {

/** Remove an Arc between two Ports.
 *
 * \ingroup engine
 */
class Disconnect : public Event
{
public:
	Disconnect(Engine&           engine,
	           SPtr<Interface>   client,
	           int32_t           id,
	           SampleCount       timestamp,
	           const Raul::Path& tail_path,
	           const Raul::Path& head_path);

	bool pre_process();
	void execute(ProcessContext& context);
	void post_process();

	class Impl {
	public:
		Impl(Engine&     e,
		     GraphImpl*  graph,
		     OutputPort* s,
		     InputPort*  d);

		bool execute(ProcessContext& context, bool set_dst_buffers);

		InputPort* head() { return _dst_input_port; }

	private:
		Engine&                 _engine;
		OutputPort*             _src_output_port;
		InputPort*              _dst_input_port;
		GraphImpl*              _graph;
		SPtr<ArcImpl>           _arc;
		Raul::Array<BufferRef>* _buffers;
	};

private:
	const Raul::Path _tail_path;
	const Raul::Path _head_path;
	GraphImpl*       _graph;
	Impl*            _impl;
	CompiledGraph*   _compiled_graph;
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_DISCONNECT_HPP
