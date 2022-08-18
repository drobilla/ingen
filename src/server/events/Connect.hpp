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

#ifndef INGEN_EVENTS_CONNECT_HPP
#define INGEN_EVENTS_CONNECT_HPP

#include "Event.hpp"
#include "PortImpl.hpp"
#include "types.hpp"

#include "ingen/Message.hpp"
#include "ingen/Properties.hpp"
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

/** Make an Arc between two Ports.
 *
 * \ingroup engine
 */
class Connect : public Event
{
public:
	Connect(Engine&                           engine,
	        const std::shared_ptr<Interface>& client,
	        SampleCount                       timestamp,
	        const ingen::Connect&             msg);

	~Connect() override;

	bool pre_process(PreProcessContext& ctx) override;
	void execute(RunContext& ctx) override;
	void post_process() override;
	void undo(Interface& target) override;

private:
	const ingen::Connect                _msg;
	GraphImpl*                          _graph{nullptr};
	InputPort*                          _head{nullptr};
	raul::managed_ptr<CompiledGraph>    _compiled_graph;
	std::shared_ptr<ArcImpl>            _arc;
	raul::managed_ptr<PortImpl::Voices> _voices;
	Properties                          _tail_remove;
	Properties                          _tail_add;
	Properties                          _head_remove;
	Properties                          _head_add;
};

} // namespace events
} // namespace server
} // namespace ingen

#endif // INGEN_EVENTS_CONNECT_HPP
