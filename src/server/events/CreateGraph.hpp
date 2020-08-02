/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_EVENTS_CREATEGRAPH_HPP
#define INGEN_EVENTS_CREATEGRAPH_HPP

#include "ClientUpdate.hpp"
#include "CompiledGraph.hpp"
#include "Event.hpp"

#include "raul/Maid.hpp"

#include <cstdint>
#include <list>
#include <memory>

namespace ingen {
namespace server {

class GraphImpl;

namespace events {

/** Creates a new Graph.
 *
 * \ingroup engine
 */
class CreateGraph : public Event
{
public:
	CreateGraph(Engine&                           engine,
	            const std::shared_ptr<Interface>& client,
	            int32_t                           id,
	            SampleCount                       timestamp,
	            const Raul::Path&                 path,
	            const Properties&                 properties);

	bool pre_process(PreProcessContext& ctx) override;
	void execute(RunContext& ctx) override;
	void post_process() override;
	void undo(Interface& target) override;

	GraphImpl* graph() { return _graph; }

private:
	void build_child_events();

	const Raul::Path                  _path;
	Properties                        _properties;
	ClientUpdate                      _update;
	GraphImpl*                        _graph;
	GraphImpl*                        _parent;
	Raul::managed_ptr<CompiledGraph>  _compiled_graph;
	std::list<std::unique_ptr<Event>> _child_events;
};

} // namespace events
} // namespace server
} // namespace ingen

#endif // INGEN_EVENTS_CREATEGRAPH_HPP
