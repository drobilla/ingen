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

#include "Event.hpp"
#include "PortImpl.hpp"
#include "types.hpp"

#include "ingen/Message.hpp"
#include "raul/Maid.hpp"

#include <memory>

namespace ingen {

class Interface;

namespace server {

class ArcImpl;
class CompiledGraph;
class Engine;
class GraphImpl;
class InputPort;
class PreProcessContext;
class RunContext;

namespace events {

/** Remove an Arc between two Ports.
 *
 * \ingroup engine
 */
class Disconnect : public Event
{
public:
	Disconnect(Engine&                           engine,
	           const std::shared_ptr<Interface>& client,
	           SampleCount                       timestamp,
	           const ingen::Disconnect&          msg);

	~Disconnect() override;

	bool pre_process(PreProcessContext& ctx) override;
	void execute(RunContext& ctx) override;
	void post_process() override;
	void undo(Interface& target) override;

	class Impl
	{
	public:
		Impl(Engine& e, GraphImpl* graph, PortImpl* t, InputPort* h);

		bool execute(RunContext& ctx, bool set_head_buffers);

		PortImpl*  tail() { return _tail; }
		InputPort* head() { return _head; }

	private:
		Engine&                             _engine;
		PortImpl*                           _tail;
		InputPort*                          _head;
		std::shared_ptr<ArcImpl>            _arc;
		raul::managed_ptr<PortImpl::Voices> _voices;
	};

private:
	const ingen::Disconnect          _msg;
	GraphImpl*                       _graph{nullptr};
	std::unique_ptr<Impl>            _impl;
	raul::managed_ptr<CompiledGraph> _compiled_graph;
};

} // namespace events
} // namespace server
} // namespace ingen

#endif // INGEN_EVENTS_DISCONNECT_HPP
