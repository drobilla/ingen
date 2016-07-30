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

#include "ingen/AtomReader.hpp"

#include "Engine.hpp"
#include "EventWriter.hpp"
#include "Undo.hpp"

namespace Ingen {
namespace Server {
namespace Events {

Undo::Undo(Engine&          engine,
           SPtr<Interface>  client,
           int32_t          id,
           SampleCount      timestamp,
           bool             is_redo)
	: Event(engine, client, id, timestamp)
	, _is_redo(is_redo)
{}

bool
Undo::pre_process()
{
	UndoStack* stack = _is_redo ? _engine.redo_stack() : _engine.undo_stack();
	Event::Mode mode = _is_redo ? Event::Mode::REDO    : Event::Mode::UNDO;

	if (stack->empty()) {
		return Event::pre_process_done(Status::NOT_FOUND);
	}

	_entry = stack->pop();
	_engine.interface()->set_event_mode(mode);
	if (_entry.events.size() > 1) {
		_engine.interface()->bundle_begin();
	}

	for (const LV2_Atom* ev : _entry.events) {
		_engine.atom_interface()->write(ev);
	}

	if (_entry.events.size() > 1) {
		_engine.interface()->bundle_end();
	}
	_engine.interface()->set_event_mode(mode);

	return Event::pre_process_done(Status::SUCCESS);
}

void
Undo::execute(ProcessContext& context)
{
}

void
Undo::post_process()
{
	respond();
}

} // namespace Events
} // namespace Server
} // namespace Ingen
