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

#ifndef INGEN_EVENTS_CREATEBLOCK_HPP
#define INGEN_EVENTS_CREATEBLOCK_HPP

#include "ingen/Resource.hpp"

#include "Event.hpp"
#include "events/Get.hpp"

namespace Ingen {
namespace Server {

class BlockImpl;
class CompiledGraph;
class GraphImpl;

namespace Events {

/** An event to load a Block and insert it into a Graph.
 *
 * \ingroup engine
 */
class CreateBlock : public Event
{
public:
	CreateBlock(Engine&                     engine,
	            SPtr<Interface>             client,
	            int32_t                     id,
	            SampleCount                 timestamp,
	            const Raul::Path&           block_path,
	            const Resource::Properties& properties);

	~CreateBlock();

	bool pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	Raul::Path            _path;
	Resource::Properties  _properties;
	Events::Get::Response _update;
	GraphImpl*            _graph;
	BlockImpl*            _block;
	CompiledGraph*        _compiled_graph;
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_CREATEBLOCK_HPP
