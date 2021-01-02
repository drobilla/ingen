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

#ifndef INGEN_EVENTS_MARK_HPP
#define INGEN_EVENTS_MARK_HPP

#include "Event.hpp"
#include "types.hpp"

#include "raul/Maid.hpp"

#include <map>
#include <memory>

namespace ingen {

class Interface;
struct BundleBegin;
struct BundleEnd;

namespace server {

class CompiledGraph;
class Engine;
class GraphImpl;
class PreProcessContext;
class RunContext;

namespace events {

/** Delineate the start or end of a bundle of events.
 *
 * This is used to mark the ends of an undo transaction, so a single undo can
 * undo the effects of many events (such as a paste or a graph load).
 *
 * \ingroup engine
 */
class Mark : public Event
{
public:
	Mark(Engine&                           engine,
	     const std::shared_ptr<Interface>& client,
	     SampleCount                       timestamp,
	     const ingen::BundleBegin&         msg);

	Mark(Engine&                           engine,
	     const std::shared_ptr<Interface>& client,
	     SampleCount                       timestamp,
	     const ingen::BundleEnd&           msg);

	~Mark() override;

	void mark(PreProcessContext& ctx) override;
	bool pre_process(PreProcessContext& ctx) override;
	void execute(RunContext& ctx) override;
	void post_process() override;

	Execution get_execution() const override;

private:
	enum class Type { BUNDLE_BEGIN, BUNDLE_END };

	using CompiledGraphs =
	    std::map<GraphImpl*, raul::managed_ptr<CompiledGraph>>;

	CompiledGraphs _compiled_graphs;
	Type           _type;
	int            _depth;
};

} // namespace events
} // namespace server
} // namespace ingen

#endif // INGEN_EVENTS_MARK_HPP
