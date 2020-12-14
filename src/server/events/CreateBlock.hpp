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

#ifndef INGEN_EVENTS_CREATEBLOCK_HPP
#define INGEN_EVENTS_CREATEBLOCK_HPP

#include "ClientUpdate.hpp"
#include "Event.hpp"
#include "types.hpp"

#include "raul/Maid.hpp"
#include "raul/Path.hpp"

#include <cstdint>
#include <memory>

namespace ingen {

class Interface;
class Properties;

namespace server {

class BlockImpl;
class CompiledGraph;
class Engine;
class GraphImpl;
class PreProcessContext;
class RunContext;

namespace events {

/** An event to load a Block and insert it into a Graph.
 *
 * \ingroup engine
 */
class CreateBlock : public Event
{
public:
	CreateBlock(Engine&                           engine,
	            const std::shared_ptr<Interface>& client,
	            int32_t                           id,
	            SampleCount                       timestamp,
	            const Raul::Path&                 path,
	            Properties&                       properties);

	~CreateBlock() override;

	bool pre_process(PreProcessContext& ctx) override;
	void execute(RunContext& ctx) override;
	void post_process() override;
	void undo(Interface& target) override;

private:
	Raul::Path                       _path;
	Properties&                      _properties;
	ClientUpdate                     _update;
	GraphImpl*                       _graph;
	BlockImpl*                       _block;
	Raul::managed_ptr<CompiledGraph> _compiled_graph;
};

} // namespace events
} // namespace server
} // namespace ingen

#endif // INGEN_EVENTS_CREATEBLOCK_HPP
