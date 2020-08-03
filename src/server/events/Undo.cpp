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

#include "Undo.hpp"

#include "Engine.hpp"
#include "EventWriter.hpp"

#include "ingen/AtomReader.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Message.hpp"
#include "ingen/Status.hpp"
#include "lv2/atom/atom.h"

#include <deque>
#include <memory>

namespace ingen {
namespace server {
namespace events {

Undo::Undo(Engine&                           engine,
           const std::shared_ptr<Interface>& client,
           SampleCount                       timestamp,
           const ingen::Undo&                msg)
	: Event(engine, client, msg.seq, timestamp)
	, _is_redo(false)
{}

Undo::Undo(Engine&                           engine,
           const std::shared_ptr<Interface>& client,
           SampleCount                       timestamp,
           const ingen::Redo&                msg)
	: Event(engine, client, msg.seq, timestamp)
	, _is_redo(true)
{}

bool
Undo::pre_process(PreProcessContext&)
{
	const std::unique_ptr<UndoStack>& stack =
	    _is_redo ? _engine.redo_stack() : _engine.undo_stack();

	const Event::Mode mode = _is_redo ? Event::Mode::REDO : Event::Mode::UNDO;

	if (stack->empty()) {
		return Event::pre_process_done(Status::NOT_FOUND);
	}

	const Event::Mode orig_mode = _engine.event_writer()->get_event_mode();
	_entry = stack->pop();
	_engine.event_writer()->set_event_mode(mode);
	if (_entry.events.size() > 1) {
		_engine.interface()->bundle_begin();
	}

	for (const LV2_Atom* ev : _entry.events) {
		_engine.atom_interface()->write(ev);
	}

	if (_entry.events.size() > 1) {
		_engine.interface()->bundle_end();
	}
	_engine.event_writer()->set_event_mode(orig_mode);

	return Event::pre_process_done(Status::SUCCESS);
}

void
Undo::execute(RunContext&)
{
}

void
Undo::post_process()
{
	respond();
}

} // namespace events
} // namespace server
} // namespace ingen
