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
#include "ClearPatchEvent.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "PatchImpl.hpp"
#include "ClientBroadcaster.hpp"
#include "util.hpp"
#include "EngineStore.hpp"
#include "PortImpl.hpp"
#include "NodeImpl.hpp"
#include "ConnectionImpl.hpp"
#include "QueuedEventSource.hpp"
#include "AudioDriver.hpp"
#include "MidiDriver.hpp"

namespace Ingen {


ClearPatchEvent::ClearPatchEvent(Engine& engine, SharedPtr<Responder> responder, FrameTime time, QueuedEventSource* source, const string& patch_path)
	: QueuedEvent(engine, responder, time, true, source)
	, _patch_path(patch_path)
	, _driver_port(NULL)
	, _process(false)
	, _ports_array(NULL)
	, _compiled_patch(NULL)
{
}


void
ClearPatchEvent::pre_process()
{
	EngineStore::Objects::iterator patch_iterator = _engine.engine_store()->find(_patch_path);
	
	if (patch_iterator != _engine.engine_store()->end()) {
		_patch = PtrCast<PatchImpl>(patch_iterator->second);
		if (_patch) {
			_process = _patch->enabled();
			_removed_table = _engine.engine_store()->remove_children(patch_iterator);
			_patch->nodes().clear();
			_patch->connections().clear();
			_ports_array = _patch->build_ports_array();
			if (_patch->enabled())
				_compiled_patch = _patch->compile();
		}
	}

	QueuedEvent::pre_process();
}


void
ClearPatchEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_patch && _removed_table) {
		_patch->disable();
		
		if (_patch->compiled_patch() != NULL) {
			_engine.maid()->push(_patch->compiled_patch());
			_patch->compiled_patch(NULL);
		}
		
		_patch->clear_ports();
		_patch->connections().clear();
		_patch->compiled_patch(_compiled_patch);
		Raul::Array<PortImpl*>* old_ports = _patch->external_ports();
		_patch->external_ports(_ports_array);
		_ports_array = old_ports;

		// Remove driver ports, if necessary
		if (_patch->parent() == NULL) {
			for (EngineStore::Objects::iterator i = _removed_table->begin(); i != _removed_table->end(); ++i) {
				SharedPtr<PortImpl> port = PtrCast<PortImpl>(i->second);
				if (port && port->type() == DataType::AUDIO)
					_driver_port = _engine.audio_driver()->remove_port(port->path());
				else if (port && port->type() == DataType::EVENT)
					_driver_port = _engine.midi_driver()->remove_port(port->path());
			}
		}
	}
}


void
ClearPatchEvent::post_process()
{	
	if (_patch != NULL) {
		delete _ports_array;
		delete _driver_port;
		
		// Restore patch's run state
		if (_process)
			_patch->enable();
		else
			_patch->disable();

		// Make sure everything's sane
		assert(_patch->nodes().size() == 0);
		assert(_patch->num_ports() == 0);
		assert(_patch->connections().size() == 0);
	
		// Reply
		_responder->respond_ok();
		_engine.broadcaster()->send_patch_cleared(_patch_path);
	} else {
		_responder->respond_error(string("Patch ") + _patch_path + " not found");
	}
	
	_source->unblock(); // FIXME: can be done earlier in execute?
}


} // namespace Ingen

