/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

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
#include "CompiledGraph.hpp"
#include "Event.hpp"
#include "GraphImpl.hpp"
#include "types.hpp"

namespace Raul {
template <typename T> class Array;
}

namespace ingen {
namespace server {

class InputPort;
class PortImpl;

namespace events {

/** Remove an Arc between two Ports.
 *
 * \ingroup engine
 */
class Disconnect : public Event
{
public:
	Disconnect(Engine&                  engine,
	           SPtr<Interface>          client,
	           SampleCount              timestamp,
	           const ingen::Disconnect& msg);

	bool pre_process(PreProcessContext& ctx) override;
	void execute(RunContext& context) override;
	void post_process() override;
	void undo(Interface& target) override;

	class Impl {
	public:
		Impl(Engine& e, GraphImpl* graph, PortImpl* t, InputPort* h);

		bool execute(RunContext& context, bool set_head_buffers);

		inline PortImpl*  tail() { return _tail; }
		inline InputPort* head() { return _head; }

	private:
		Engine&                _engine;
		PortImpl*              _tail;
		InputPort*             _head;
		SPtr<ArcImpl>          _arc;
		MPtr<PortImpl::Voices> _voices;
	};

private:
	const ingen::Disconnect _msg;
	GraphImpl*              _graph;
	UPtr<Impl>              _impl;
	MPtr<CompiledGraph>     _compiled_graph;
};

} // namespace events
} // namespace server
} // namespace ingen

#endif // INGEN_EVENTS_DISCONNECT_HPP
