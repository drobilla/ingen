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

#include "ingen/Store.hpp"

#include "Event.hpp"
#include "GraphImpl.hpp"
#include "ControlBindings.hpp"

namespace Raul {
template<typename T> class Array;
}

namespace Ingen {
namespace Server {

class BlockImpl;
class CompiledGraph;
class DuplexPort;
class EnginePort;
class PortImpl;

namespace Events {

class DisconnectAll;

/** Delete a graph object.
 * \ingroup engine
 */
class Delete : public Event
{
public:
	Delete(Engine&          engine,
	       SPtr<Interface>  client,
	       int32_t          id,
	       FrameTime        timestamp,
	       const Raul::URI& uri);

	~Delete();

	bool pre_process();
	void execute(ProcessContext& context);
	void post_process();
	void undo(Interface& target);

private:
	Raul::URI               _uri;
	Raul::Path              _path;
	SPtr<BlockImpl>         _block; ///< Non-NULL iff a block
	SPtr<DuplexPort>        _port; ///< Non-NULL iff a port
	EnginePort*             _engine_port;
	Raul::Array<PortImpl*>* _ports_array; ///< New (external) ports for Graph
	CompiledGraph*          _compiled_graph; ///< Graph's new process order
	DisconnectAll*          _disconnect_event;

	SPtr<ControlBindings::Bindings> _removed_bindings;
	Store::Objects                  _removed_objects;
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_DELETE_HPP
