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

#include <raul/Maid.hpp>
#include <raul/Path.hpp>
#include "CreatePatchEvent.hpp"
#include "Responder.hpp"
#include "PatchImpl.hpp"
#include "NodeImpl.hpp"
#include "PluginImpl.hpp"
#include "Engine.hpp"
#include "ClientBroadcaster.hpp"
#include "AudioDriver.hpp"
#include "EngineStore.hpp"

namespace Ingen {


CreatePatchEvent::CreatePatchEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& path, int poly)
: QueuedEvent(engine, responder, timestamp),
  _path(path),
  _patch(NULL),
  _parent(NULL),
  _compiled_patch(NULL),
  _poly(poly),
  _error(NO_ERROR)
{
}


void
CreatePatchEvent::pre_process()
{
	if (!Path::is_valid(_path)) {
		_error = INVALID_PATH;
		QueuedEvent::pre_process();
		return;
	}

	if (_path == "/" || _engine.engine_store()->find_object(_path) != NULL) {
		_error = OBJECT_EXISTS;
		QueuedEvent::pre_process();
		return;
	}

	if (_poly < 1) {
		_error = INVALID_POLY;
		QueuedEvent::pre_process();
		return;
	}
	
	const Path& path = (const Path&)_path;

	_parent = _engine.engine_store()->find_patch(path.parent());
	if (_parent == NULL) {
		_error = PARENT_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}
	
	uint32_t poly = 1;
	if (_parent != NULL && _poly > 1 && _poly == static_cast<int>(_parent->internal_polyphony()))
		poly = _poly;
	
	_patch = new PatchImpl(_engine, path.name(), poly, _parent, _engine.audio_driver()->sample_rate(), _engine.audio_driver()->buffer_size(), _poly);
		
	if (_parent != NULL) {
		_parent->add_node(new PatchImpl::Nodes::Node(_patch));

		if (_parent->enabled())
			_compiled_patch = _parent->compile();
	}
	
	_patch->activate();
	
	// Insert into EngineStore
	//_patch->add_to_store(_engine.engine_store());
	_engine.engine_store()->add(_patch);
	
	QueuedEvent::pre_process();
}


void
CreatePatchEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_patch != NULL) {
		if (_parent == NULL) {
			assert(_path == "/");
			assert(_patch->parent_patch() == NULL);
			_engine.audio_driver()->set_root_patch(_patch);
		} else {
			assert(_parent != NULL);
			assert(_path != "/");
			
			if (_parent->compiled_patch() != NULL)
				_engine.maid()->push(_parent->compiled_patch());
			_parent->compiled_patch(_compiled_patch);
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
			
		} else if (_error == INVALID_PATH) {
			string msg = "Attempt to create patch with illegal path ";
			msg.append(_path);
			_responder->respond_error(msg);
		} else if (_error == OBJECT_EXISTS) {
			string msg = "Unable to create patch: ";
			msg.append(_path).append(" already exists.");
			_responder->respond_error(msg);
		} else if (_error == PARENT_NOT_FOUND) {
			string msg = "Unable to create patch: Parent ";
			msg.append(Path(_path).parent()).append(" not found.");
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

