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

#include "Engine.hpp"
#include "UndoStack.hpp"
#include "events/Mark.hpp"

namespace Ingen {
namespace Server {
namespace Events {

Mark::Mark(Engine&         engine,
           SPtr<Interface> client,
           int32_t         id,
           SampleCount     timestamp,
           Type            type)
	: Event(engine, client, id, timestamp)
	, _type(type)
{}

bool
Mark::pre_process()
{
	UndoStack* const stack = ((_mode == Mode::UNDO)
	                          ? _engine.redo_stack()
	                          : _engine.undo_stack());

	switch (_type) {
	case Type::BUNDLE_START:
		stack->start_entry();
		break;
	case Type::BUNDLE_END:
		stack->finish_entry();
		break;
	}

	return Event::pre_process_done(Status::SUCCESS);
}

void
Mark::execute(ProcessContext& context)
{}

void
Mark::post_process()
{
	respond();
}

} // namespace Events
} // namespace Server
} // namespace Ingen
