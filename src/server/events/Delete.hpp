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

#ifndef INGEN_EVENTS_DELETE_HPP
#define INGEN_EVENTS_DELETE_HPP

#include <map>
#include <vector>

#include "ingen/Store.hpp"

#include "CompiledGraph.hpp"
#include "ControlBindings.hpp"
#include "Event.hpp"
#include "GraphImpl.hpp"

namespace Raul {
template<typename T> class Array;
}

namespace ingen {
namespace server {

class BlockImpl;
class DuplexPort;
class EnginePort;
class PortImpl;

namespace events {

class DisconnectAll;

/** Delete a graph object.
 * \ingroup engine
 */
class Delete : public Event
{
public:
	Delete(Engine&           engine,
	       SPtr<Interface>   client,
	       FrameTime         timestamp,
	       const ingen::Del& msg);

	~Delete();

	bool pre_process(PreProcessContext& ctx);
	void execute(RunContext& context);
	void post_process();
	void undo(Interface& target);

private:
	using IndexChange  = std::pair<uint32_t, uint32_t>;
	using IndexChanges = std::map<Raul::Path, IndexChange>;

	const ingen::Del        _msg;
	Raul::Path              _path;
	SPtr<BlockImpl>         _block; ///< Non-NULL iff a block
	SPtr<DuplexPort>        _port; ///< Non-NULL iff a port
	EnginePort*             _engine_port;
	MPtr<GraphImpl::Ports>  _ports_array; ///< New (external) ports for Graph
	MPtr<CompiledGraph>     _compiled_graph; ///< Graph's new process order
	DisconnectAll*          _disconnect_event;
	Store::Objects          _removed_objects;
	IndexChanges            _port_index_changes;

	std::vector<ControlBindings::Binding*> _removed_bindings;
};

} // namespace events
} // namespace server
} // namespace ingen

#endif // INGEN_EVENTS_DELETE_HPP
