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

#include "RequestObjectEvent.hpp"
#include <string>
#include "interface/ClientInterface.hpp"
#include "interface/Responder.hpp"
#include "Engine.hpp"
#include "ObjectStore.hpp"
#include "ClientBroadcaster.hpp"
#include "Patch.hpp"
#include "Node.hpp"
#include "Port.hpp"
#include "ObjectSender.hpp"

using std::string;

namespace Ingen {


RequestObjectEvent::RequestObjectEvent(Engine& engine, SharedPtr<Shared::Responder> responder, SampleCount timestamp, const string& path)
: QueuedEvent(engine, responder, timestamp),
  _path(path),
  _object(NULL)
{
}


void
RequestObjectEvent::pre_process()
{
	_client = _engine.broadcaster()->client(_responder->client_key());
	_object = _engine.object_store()->find(_path);

	QueuedEvent::pre_process();
}


void
RequestObjectEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);
	assert(_time >= start && _time <= end);
}


void
RequestObjectEvent::post_process()
{
	if (!_object) {
		_responder->respond_error("Unable to find object requested.");
	
	} else if (_client) {	
		Patch* const patch = dynamic_cast<Patch*>(_object);
		if (patch) {
			_responder->respond_ok();
			ObjectSender::send_patch(_client.get(), patch, true);
			return;
		}
		
		Node* const node = dynamic_cast<Node*>(_object);
		if (node) {
			_responder->respond_ok();
			ObjectSender::send_node(_client.get(), node, true);
			return;
		}
		
		Port* const port = dynamic_cast<Port*>(_object);
		if (port) {
			_responder->respond_ok();
			ObjectSender::send_port(_client.get(), port);
			return;
		}
		
		_responder->respond_error("Object of unknown type requested.");

	} else {
		_responder->respond_error("Unable to find client to send object.");
	}
}


} // namespace Ingen

