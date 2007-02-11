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

#include "CreatePatchEvent.h"
#include "Responder.h"
#include "Patch.h"
#include "Node.h"
#include "Tree.h"
#include "Plugin.h"
#include "Engine.h"
#include <raul/Maid.h>
#include "ClientBroadcaster.h"
#include "AudioDriver.h"
#include "raul/Path.h"
#include "ObjectStore.h"

namespace Ingen {


CreatePatchEvent::CreatePatchEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& path, int poly)
: QueuedEvent(engine, responder, timestamp),
  _path(path),
  _patch(NULL),
  _parent(NULL),
  _process_order(NULL),
  _poly(poly),
  _error(NO_ERROR)
{
}


void
CreatePatchEvent::pre_process()
{
	if (_path == "/" || _engine.object_store()->find(_path) != NULL) {
		_error = OBJECT_EXISTS;
		QueuedEvent::pre_process();
		return;
	}

	if (_poly < 1) {
		_error = INVALID_POLY;
		QueuedEvent::pre_process();
		return;
	}
	
	_parent = _engine.object_store()->find_patch(_path.parent());
	if (_parent == NULL) {
		_error = PARENT_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}
	
	size_t poly = 1;
	if (_parent != NULL && _poly > 1 && _poly == static_cast<int>(_parent->internal_poly()))
		poly = _poly;
	
	_patch = new Patch(_path.name(), poly, _parent, _engine.audio_driver()->sample_rate(), _engine.audio_driver()->buffer_size(), _poly);
		
	if (_parent != NULL) {
		_parent->add_node(new Raul::ListNode<Node*>(_patch));

		if (_parent->enabled())
			_process_order = _parent->build_process_order();
	}
	
	_patch->activate();
	
	// Insert into ObjectStore
	_patch->add_to_store(_engine.object_store());
	
	QueuedEvent::pre_process();
}


void
CreatePatchEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);

	if (_patch != NULL) {
		if (_parent == NULL) {
			assert(_path == "/");
			assert(_patch->parent_patch() == NULL);
			_engine.audio_driver()->set_root_patch(_patch);
		} else {
			assert(_parent != NULL);
			assert(_path != "/");
			
			if (_parent->process_order() != NULL)
				_engine.maid()->push(_parent->process_order());
			_parent->process_order(_process_order);
		}
	}
}


void
CreatePatchEvent::post_process()
{
	if (_responder.get()) {
		if (_error == NO_ERROR) {
			
			_responder->respond_ok();
			
			// Don't send ports/nodes that have been added since prepare()
			// (otherwise they would be sent twice)
			_engine.broadcaster()->send_patch(_patch, false);
			
		} else if (_error == OBJECT_EXISTS) {
			string msg = "Unable to create patch: ";
			msg += _path += " already exists.";
			_responder->respond_error(msg);
		} else if (_error == PARENT_NOT_FOUND) {
			string msg = "Unable to create patch: Parent ";
			msg += _path.parent() += " not found.";
			_responder->respond_error(msg);
		} else if (_error == INVALID_POLY) {
			string msg = "Unable to create patch ";
			msg.append(_path).append(": ").append("Invalid polyphony respondered.");
			_responder->respond_error(msg);
		} else {
			_responder->respond_error("Unable to load patch.");
		}
	}
}


} // namespace Ingen

