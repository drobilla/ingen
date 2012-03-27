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

#include "ClientBroadcaster.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "NodeImpl.hpp"
#include "PatchImpl.hpp"
#include "events/Move.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {
namespace Events {

Move::Move(Engine&     engine,
           Interface*  client,
           int32_t     id,
           SampleCount timestamp,
           const Path& path,
           const Path& new_path)
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

void
Move::pre_process()
{
	Glib::RWLock::WriterLock lock(_engine.engine_store()->lock());

	if (!_old_path.parent().is_parent_of(_new_path)) {
		_status = PARENT_DIFFERS;
		Event::pre_process();
		return;
	}
	_store_iterator = _engine.engine_store()->find(_old_path);
	if (_store_iterator == _engine.engine_store()->end())  {
		_status = NOT_FOUND;
		Event::pre_process();
		return;
	}

	if (_engine.engine_store()->find_object(_new_path))  {
		_status = EXISTS;
		Event::pre_process();
		return;
	}

	SharedPtr< Table<Path, SharedPtr<GraphObject> > > removed
			= _engine.engine_store()->remove(_store_iterator);

	assert(removed->size() > 0);

	for (Table<Path, SharedPtr<GraphObject> >::iterator i = removed->begin(); i != removed->end(); ++i) {
		const Path& child_old_path = i->first;
		assert(Path::descendant_comparator(_old_path, child_old_path));

		Path child_new_path;
		if (child_old_path == _old_path)
			child_new_path = _new_path;
		else
			child_new_path = Path(_new_path).base() + child_old_path.substr(_old_path.length()+1);

		PtrCast<GraphObjectImpl>(i->second)->set_path(child_new_path);
		i->first = child_new_path;
	}

	_engine.engine_store()->add(*removed.get());

	Event::pre_process();
}

void
Move::execute(ProcessContext& context)
{
	Event::execute(context);

	SharedPtr<PortImpl> port = PtrCast<PortImpl>(_store_iterator->second);
	if (port && port->parent()->parent() == NULL) {
		DriverPort* driver_port = _engine.driver()->driver_port(_new_path);
		if (driver_port)
			driver_port->move(_new_path);
	}
}

void
Move::post_process()
{
	respond(_status);
	if (!_status) {
		_engine.broadcaster()->move(_old_path, _new_path);
	}
}

} // namespace Server
} // namespace Ingen
} // namespace Events
