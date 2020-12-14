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

#ifndef INGEN_EVENTS_COPY_HPP
#define INGEN_EVENTS_COPY_HPP

#include "Event.hpp"
#include "types.hpp"

#include "ingen/Message.hpp"
#include "raul/Maid.hpp"

#include <memory>

namespace ingen {

class Interface;

namespace server {

class BlockImpl;
class CompiledGraph;
class Engine;
class GraphImpl;
class PreProcessContext;
class RunContext;

namespace events {

/** Copy a graph object to a new path.
 * \ingroup engine
 */
class Copy : public Event
{
public:
	Copy(Engine&                           engine,
	     const std::shared_ptr<Interface>& client,
	     SampleCount                       timestamp,
	     const ingen::Copy&                msg);

	~Copy() override;

	bool pre_process(PreProcessContext& ctx) override;
	void execute(RunContext& ctx) override;
	void post_process() override;
	void undo(Interface& target) override;

private:
	bool engine_to_engine(PreProcessContext& ctx);
	bool engine_to_filesystem(PreProcessContext& ctx);
	bool filesystem_to_engine(PreProcessContext& ctx);

	const ingen::Copy                _msg;
	std::shared_ptr<BlockImpl>       _old_block;
	GraphImpl*                       _parent;
	BlockImpl*                       _block;
	Raul::managed_ptr<CompiledGraph> _compiled_graph;
};

} // namespace events
} // namespace server
} // namespace ingen

#endif // INGEN_EVENTS_COPY_HPP
