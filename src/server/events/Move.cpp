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

#include "Broadcaster.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "events/Move.hpp"

#include "ingen/Interface.hpp"
#include "ingen/Status.hpp"
#include "ingen/Store.hpp"
#include "raul/Path.hpp"

#include <map>
#include <memory>
#include <mutex>

namespace ingen {
namespace server {

class EnginePort;

namespace events {

Move::Move(Engine&                           engine,
           const std::shared_ptr<Interface>& client,
           SampleCount                       timestamp,
           const ingen::Move&                msg)
	: Event(engine, client, msg.seq, timestamp)
	, _msg(msg)
{
}

bool
Move::pre_process(PreProcessContext&)
{
	std::lock_guard<Store::Mutex> lock(_engine.store()->mutex());

	if (!_msg.old_path.parent().is_parent_of(_msg.new_path)) {
		return Event::pre_process_done(Status::PARENT_DIFFERS, _msg.new_path);
	}

	const Store::iterator i = _engine.store()->find(_msg.old_path);
	if (i == _engine.store()->end()) {
		return Event::pre_process_done(Status::NOT_FOUND, _msg.old_path);
	}

	if (_engine.store()->find(_msg.new_path) != _engine.store()->end()) {
		return Event::pre_process_done(Status::EXISTS, _msg.new_path);
	}

	EnginePort* eport = _engine.driver()->get_port(_msg.old_path);
	if (eport) {
		_engine.driver()->rename_port(_msg.old_path, _msg.new_path);
	}

	_engine.store()->rename(i, _msg.new_path);

	return Event::pre_process_done(Status::SUCCESS);
}

void
Move::execute(RunContext&)
{
}

void
Move::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS) {
		_engine.broadcaster()->message(_msg);
	}
}

void
Move::undo(Interface& target)
{
	target.move(_msg.new_path, _msg.old_path);
}

} // namespace events
} // namespace server
} // namespace ingen
