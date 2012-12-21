/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <glibmm/thread.h>

#include "ingen/Store.hpp"
#include "raul/Path.hpp"

#include "BlockImpl.hpp"
#include "Broadcaster.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EnginePort.hpp"
#include "GraphImpl.hpp"
#include "events/Move.hpp"

namespace Ingen {
namespace Server {
namespace Events {

Move::Move(Engine&              engine,
           SharedPtr<Interface> client,
           int32_t              id,
           SampleCount          timestamp,
           const Raul::Path&    path,
           const Raul::Path&    new_path)
	: Event(engine, client, id, timestamp)
	, _old_path(path)
	, _new_path(new_path)
{
}

Move::~Move()
{
}

bool
Move::pre_process()
{
	Glib::RWLock::WriterLock lock(_engine.store()->lock());

	if (!_old_path.parent().is_parent_of(_new_path)) {
		return Event::pre_process_done(PARENT_DIFFERS, _new_path);
	}

	const Store::iterator i = _engine.store()->find(_old_path);
	if (i == _engine.store()->end()) {
		return Event::pre_process_done(NOT_FOUND, _old_path);
	}

	if (_engine.store()->find(_new_path) != _engine.store()->end()) {
		return Event::pre_process_done(EXISTS, _new_path);
	}

	EnginePort* eport = _engine.driver()->get_port(_old_path);
	if (eport) {
		_engine.driver()->rename_port(_old_path, _new_path);
	}

	_engine.store()->rename(i, _new_path);

	return Event::pre_process_done(SUCCESS);
}

void
Move::execute(ProcessContext& context)
{
}

void
Move::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (!respond()) {
		_engine.broadcaster()->move(_old_path, _new_path);
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
