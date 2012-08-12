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

#include "raul/Path.hpp"

#include "Broadcaster.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EnginePort.hpp"
#include "EngineStore.hpp"
#include "NodeImpl.hpp"
#include "PatchImpl.hpp"
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
	, _parent_patch(NULL)
	, _store_iterator(engine.engine_store()->end())
{
}

Move::~Move()
{
}

bool
Move::pre_process()
{
	Glib::RWLock::WriterLock lock(_engine.engine_store()->lock());

	if (!_old_path.parent().is_parent_of(_new_path)) {
		return Event::pre_process_done(PARENT_DIFFERS, _new_path);
	}

	_store_iterator = _engine.engine_store()->find(_old_path);
	if (_store_iterator == _engine.engine_store()->end()) {
		return Event::pre_process_done(NOT_FOUND, _old_path);
	}

	if (_engine.engine_store()->find_object(_new_path)) {
		return Event::pre_process_done(EXISTS, _new_path);
	}

	SharedPtr< Raul::Table< Raul::Path, SharedPtr<GraphObject> > > removed
		= _engine.engine_store()->remove(_store_iterator);

	assert(removed->size() > 0);

	for (Raul::Table< Raul::Path, SharedPtr<GraphObject> >::iterator i = removed->begin(); i != removed->end(); ++i) {
		const Raul::Path& child_old_path = i->first;
		assert(Raul::Path::descendant_comparator(_old_path, child_old_path));

		Raul::Path child_new_path;
		if (child_old_path == _old_path)
			child_new_path = _new_path;
		else
			child_new_path = Raul::Path(_new_path).base()
				+ child_old_path.substr(_old_path.length() + 1);

		PtrCast<GraphObjectImpl>(i->second)->set_path(child_new_path);
		i->first = child_new_path;
	}

	_engine.engine_store()->add(*removed.get());

	return Event::pre_process_done(SUCCESS);
}

void
Move::execute(ProcessContext& context)
{
	SharedPtr<PortImpl> port = PtrCast<PortImpl>(_store_iterator->second);
	if (port && port->parent()->parent() == NULL) {
		EnginePort* eport = _engine.driver()->engine_port(context, _new_path);
		if (eport) {
			eport->move(_new_path);
		}
	}
}

void
Move::post_process()
{
	if (!respond()) {
		_engine.broadcaster()->move(_old_path, _new_path);
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
