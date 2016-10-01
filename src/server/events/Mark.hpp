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

namespace Ingen {
namespace Server {

class Engine;

namespace Events {

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
	enum class Type { BUNDLE_START, BUNDLE_END };

	Mark(Engine&                     engine,
	     SPtr<Interface>             client,
	     int32_t                     id,
	     SampleCount                 timestamp,
	     Type                        type);

	~Mark();

	bool pre_process(PreProcessContext& ctx);
	void execute(RunContext& context);
	void post_process();

	Execution get_execution() const;

private:
	typedef std::map<GraphImpl*, CompiledGraph*> CompiledGraphs;

	CompiledGraphs _compiled_graphs;
	Type           _type;
	int            _depth;
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_MARK_HPP
