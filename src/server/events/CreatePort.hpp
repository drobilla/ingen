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

#ifndef INGEN_EVENTS_CREATEPORT_HPP
#define INGEN_EVENTS_CREATEPORT_HPP

#include <boost/optional.hpp>

#include "ingen/Resource.hpp"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "raul/Array.hpp"
#include "raul/Path.hpp"

#include "Event.hpp"
#include "PortType.hpp"

namespace Ingen {
namespace Server {

class DuplexPort;
class EnginePort;
class GraphImpl;
class PortImpl;

namespace Events {

/** An event to add a Port to a Graph.
 *
 * \ingroup engine
 */
class CreatePort : public Event
{
public:
	CreatePort(Engine&                     engine,
	           SPtr<Interface>             client,
	           int32_t                     id,
	           SampleCount                 timestamp,
	           const Raul::Path&           path,
	           const Resource::Properties& properties);

	bool pre_process();
	void execute(RunContext& context);
	void post_process();
	void undo(Interface& target);

private:
	enum class Flow {
		INPUT,
		OUTPUT
	};

	Raul::Path              _path;
	PortType                _port_type;
	LV2_URID                _buf_type;
	GraphImpl*              _graph;
	DuplexPort*             _graph_port;
	Raul::Array<PortImpl*>* _ports_array; ///< New external port array for Graph
	Raul::Array<PortImpl*>* _old_ports_array;
	EnginePort*             _engine_port; ///< Driver port if on the root
	Resource::Properties    _properties;
	Resource::Properties    _update;
	boost::optional<Flow>   _flow;
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_CREATEPORT_HPP
