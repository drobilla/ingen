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

#ifndef INGEN_EVENTS_MOVE_HPP
#define INGEN_EVENTS_MOVE_HPP

#include "ingen/Store.hpp"
#include "raul/Path.hpp"

#include "Event.hpp"

namespace ingen {
namespace server {

class GraphImpl;
class PortImpl;

namespace events {

/** Move a graph object to a new path.
 * \ingroup engine
 */
class Move : public Event
{
public:
	Move(Engine&            engine,
	     SPtr<Interface>    client,
	     SampleCount        timestamp,
	     const ingen::Move& msg);

	bool pre_process(PreProcessContext& ctx) override;
	void execute(RunContext& context) override;
	void post_process() override;
	void undo(Interface& target) override;

private:
	const ingen::Move _msg;
};

} // namespace events
} // namespace server
} // namespace ingen

#endif // INGEN_EVENTS_MOVE_HPP
