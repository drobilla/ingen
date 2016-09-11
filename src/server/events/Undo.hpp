/*
  This file is part of Ingen.
  Copyright 2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_EVENTS_UNDO_HPP
#define INGEN_EVENTS_UNDO_HPP

#include "Event.hpp"
#include "UndoStack.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {
namespace Events {

/** A request to undo the last change to the engine.
 *
 * \ingroup engine
 */
class Undo : public Event
{
public:
	Undo(Engine&         engine,
	     SPtr<Interface> client,
	     int32_t         id,
	     SampleCount     timestamp,
	     bool            is_redo);

	bool pre_process();
	void execute(RunContext& context);
	void post_process();

private:
	UndoStack::Entry _entry;
	bool             _is_redo;
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_UNDO_HPP
