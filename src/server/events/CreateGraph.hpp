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

#include "ingen/Resource.hpp"

#include "Event.hpp"
#include "events/Get.hpp"

namespace Ingen {
namespace Server {

class GraphImpl;
class CompiledGraph;

namespace Events {

/** Creates a new Graph.
 *
 * \ingroup engine
 */
class CreateGraph : public Event
{
public:
	CreateGraph(Engine&                     engine,
	            SPtr<Interface>             client,
	            int32_t                     id,
	            SampleCount                 timestamp,
	            const Raul::Path&           path,
	            const Resource::Properties& properties);

	bool pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	const Raul::Path      _path;
	Resource::Properties  _properties;
	Events::Get::Response _update;
	GraphImpl*            _graph;
	GraphImpl*            _parent;
	CompiledGraph*        _compiled_graph;
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_CREATEGRAPH_HPP
