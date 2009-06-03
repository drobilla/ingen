/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "raul/Path.hpp"
#include "events/Move.hpp"
#include "ClientBroadcaster.hpp"
#include "Engine.hpp"
#include "NodeImpl.hpp"
#include "EngineStore.hpp"
#include "PatchImpl.hpp"
#include "Responder.hpp"
#include "AudioDriver.hpp"
#include "MidiDriver.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Events {

using namespace Shared;


Move::Move(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const Path& path, const Path& new_path)
	: QueuedEvent(engine, responder, timestamp)
	, _old_path(path)
	, _new_path(new_path)
	, _parent_patch(NULL)
	, _store_iterator(engine.engine_store()->end())
	, _error(NO_ERROR)
{
}


Move::~Move()
{
}


void
Move::pre_process()
{
	if (!_old_path.parent().is_parent_of(_new_path)) {
		_error = PARENT_DIFFERS;
		QueuedEvent::pre_process();
		return;
	}
	_store_iterator = _engine.engine_store()->find(_old_path);
	if (_store_iterator == _engine.engine_store()->end())  {
		_error = OBJECT_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}

	if (_engine.engine_store()->find_object(_new_path))  {
		_error = OBJECT_EXISTS;
		QueuedEvent::pre_process();
		return;
	}

	SharedPtr< Table<Path, SharedPtr<Shared::GraphObject> > > removed
			= _engine.engine_store()->remove(_store_iterator);

	assert(removed->size() > 0);

	for (Table<Path, SharedPtr<Shared::GraphObject> >::iterator i = removed->begin(); i != removed->end(); ++i) {
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

	QueuedEvent::pre_process();
}


void
Move::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	SharedPtr<PortImpl> port = PtrCast<PortImpl>(_store_iterator->second);
	if (port && port->parent()->parent() == NULL) {
		DriverPort* driver_port = NULL;

		if (port->type() == DataType::AUDIO)
			driver_port = _engine.audio_driver()->driver_port(_new_path);
		else if (port->type() == DataType::EVENT)
			driver_port = _engine.midi_driver()->driver_port(_new_path);

		if (driver_port)
			driver_port->set_name(_new_path.str());
	}
}


void
Move::post_process()
{
	string msg = "Unable to rename object - ";

	if (_error == NO_ERROR) {
		_responder->respond_ok();
		_engine.broadcaster()->send_move(_old_path, _new_path);
	} else {
		if (_error == OBJECT_EXISTS)
			msg.append("Object already exists at ").append(_new_path.str());
		else if (_error == OBJECT_NOT_FOUND)
			msg.append("Could not find object ").append(_old_path.str());
		else if (_error == OBJECT_NOT_RENAMABLE)
			msg.append(_old_path.str()).append(" is not renamable");
		else if (_error == PARENT_DIFFERS)
			msg.append(_new_path.str()).append(" is a child of a different patch");

		_responder->respond_error(msg);
	}
}


} // namespace Ingen
} // namespace Events
