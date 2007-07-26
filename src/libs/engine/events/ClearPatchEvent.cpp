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

#include "ClearPatchEvent.hpp"
#include "interface/Responder.hpp"
#include "Engine.hpp"
#include "Patch.hpp"
#include "ClientBroadcaster.hpp"
#include "util.hpp"
#include "ObjectStore.hpp"
#include "Port.hpp"
#include <raul/Maid.hpp>
#include "Node.hpp"
#include "Connection.hpp"
#include "QueuedEventSource.hpp"

namespace Ingen {


ClearPatchEvent::ClearPatchEvent(Engine& engine, SharedPtr<Shared::Responder> responder, FrameTime time, QueuedEventSource* source, const string& patch_path)
: QueuedEvent(engine, responder, time, true, source),
  _patch_path(patch_path),
  _patch(NULL),
  _process(false)
{
}


void
ClearPatchEvent::pre_process()
{
	cerr << "FIXME: CLEAR PATCH\n";
#if 0
	_patch = _engine.object_store()->find_patch(_patch_path);
	
	if (_patch != NULL) {
	
		_process = _patch->enabled();

		for (Raul::List<Node*>::const_iterator i = _patch->nodes().begin(); i != _patch->nodes().end(); ++i)
			(*i)->remove_from_store();
	}
#endif

	QueuedEvent::pre_process();
}


void
ClearPatchEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);

	if (_patch != NULL) {
		_patch->disable();
		
		cerr << "FIXME: CLEAR PATCH\n";
		//for (Raul::List<Node*>::const_iterator i = _patch->nodes().begin(); i != _patch->nodes().end(); ++i)
		//	(*i)->remove_from_patch();

		if (_patch->process_order() != NULL) {
			_engine.maid()->push(_patch->process_order());
			_patch->process_order(NULL);
		}
	}
}


void
ClearPatchEvent::post_process()
{	
	if (_patch != NULL) {
		// Delete all nodes
		for (Raul::List<Node*>::iterator i = _patch->nodes().begin(); i != _patch->nodes().end(); ++i) {
			(*i)->deactivate();
			delete *i;
		}
		_patch->nodes().clear();

		// Delete all connections
		for (Raul::List<Connection*>::iterator i = _patch->connections().begin(); i != _patch->connections().end(); ++i)
			delete *i;
		_patch->connections().clear();
		
		// Restore patch's run state
		if (_process)
			_patch->enable();
		else
			_patch->disable();

		// Make sure everything's sane
		assert(_patch->nodes().size() == 0);
		assert(_patch->connections().size() == 0);
		
		// Reply
		_responder->respond_ok();
		_engine.broadcaster()->send_patch_cleared(_patch_path);
	} else {
		_responder->respond_error(string("Patch ") + _patch_path + " not found");
	}
	
	_source->unblock();
}


} // namespace Ingen

