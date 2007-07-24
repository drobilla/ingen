/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include "RenameEvent.hpp"
#include "interface/Responder.hpp"
#include "Patch.hpp"
#include "Node.hpp"
#include "Tree.hpp"
#include "Engine.hpp"
#include "ClientBroadcaster.hpp"
#include <raul/Path.hpp>
#include "ObjectStore.hpp"

namespace Ingen {


RenameEvent::RenameEvent(Engine& engine, SharedPtr<Shared::Responder> responder, SampleCount timestamp, const string& path, const string& name)
: QueuedEvent(engine, responder, timestamp),
  _old_path(path),
  _name(name),
  _new_path(_old_path.parent().base() + name),
  _parent_patch(NULL),
  _store_treenode(NULL),
  _error(NO_ERROR)
{
	/*
	if (_old_path.parent() == "/")
		_new_path = string("/") + _name;
	else
		_new_path = _old_path.parent() +"/"+ _name;*/
}


RenameEvent::~RenameEvent()
{
}


void
RenameEvent::pre_process()
{
	if (_name.find("/") != string::npos) {
		_error = INVALID_NAME;
		QueuedEvent::pre_process();
		return;
	}

	if (_engine.object_store()->find(_new_path)) {
		_error = OBJECT_EXISTS;
		QueuedEvent::pre_process();
		return;
	}
	
	GraphObject* obj = _engine.object_store()->find(_old_path);

	if (obj == NULL) {
		_error = OBJECT_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}
	
	// Renaming only works for Nodes and Patches (which are Nodes)
	/*if (obj->as_node() == NULL) {
		_error = OBJECT_NOT_RENAMABLE;
		QueuedEvent::pre_process();
		return;
	}*/
	
	if (obj != NULL) {
		obj->set_path(_new_path);
		assert(obj->path() == _new_path);
	}
	
	QueuedEvent::pre_process();
}


void
RenameEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	//cout << "Executing rename event...";
	QueuedEvent::execute(nframes, start, end);
}


void
RenameEvent::post_process()
{
	string msg = "Unable to rename object - ";
	
	if (_error == NO_ERROR) {
		_responder->respond_ok();
		_engine.broadcaster()->send_rename(_old_path, _new_path);
	} else {
		if (_error == OBJECT_EXISTS)
			msg.append("Object already exists at ").append(_new_path);
		else if (_error == OBJECT_NOT_FOUND)
			msg.append("Could not find object ").append(_old_path);
		else if (_error == OBJECT_NOT_RENAMABLE)
			msg.append(_old_path).append(" is not renamable");
		else if (_error == INVALID_NAME)
			msg.append(_name).append(" is not a valid name");

		_responder->respond_error(msg);
	}
}


} // namespace Ingen
