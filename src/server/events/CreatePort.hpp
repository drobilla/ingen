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

#include "BlockImpl.hpp"
#include "Event.hpp"
#include "PortType.hpp"

#include "lv2/urid/urid.h"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"

#include <boost/optional/optional.hpp>

#include <cstdint>
#include <memory>

namespace ingen {
namespace server {

class DuplexPort;
class EnginePort;
class GraphImpl;

namespace events {

/** An event to add a Port to a Graph.
 *
 * \ingroup engine
 */
class CreatePort : public Event
{
public:
	CreatePort(Engine&                           engine,
	           const std::shared_ptr<Interface>& client,
	           int32_t                           id,
	           SampleCount                       timestamp,
	           const Raul::Path&                 path,
	           const Properties&                 properties);

	bool pre_process(PreProcessContext& ctx) override;
	void execute(RunContext& ctx) override;
	void post_process() override;
	void undo(Interface& target) override;

private:
	enum class Flow {
		INPUT,
		OUTPUT
	};

	Raul::Path                          _path;
	PortType                            _port_type;
	LV2_URID                            _buf_type;
	GraphImpl*                          _graph;
	DuplexPort*                         _graph_port;
	Raul::managed_ptr<BlockImpl::Ports> _ports_array; ///< New external port array for Graph
	EnginePort*                         _engine_port; ///< Driver port if on the root
	Properties                          _properties;
	Properties                          _update;
	boost::optional<Flow>               _flow;
};

} // namespace events
} // namespace server
} // namespace ingen

#endif // INGEN_EVENTS_CREATEPORT_HPP
