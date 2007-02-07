/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include "DSSIConfigureEvent.h"
#include "Engine.h"
#include "Node.h"
#include "ClientBroadcaster.h"
#include "Plugin.h"
#include "ObjectStore.h"

namespace Ingen {


DSSIConfigureEvent::DSSIConfigureEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& node_path, const string& key, const string& val)
: QueuedEvent(engine, responder, timestamp),
  _node_path(node_path),
  _key(key),
  _val(val),
  _node(NULL)
{
}


void
DSSIConfigureEvent::pre_process()
{
	Node* node = _engine.object_store()->find_node(_node_path);

	if (node != NULL && node->plugin()->type() == Plugin::DSSI) {
		_node = (DSSINode*)node;
		_node->configure(_key, _val);
	}

	QueuedEvent::pre_process();
}

	
void
DSSIConfigureEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);
	// Nothing.
}


void
DSSIConfigureEvent::post_process()
{
	if (_node == NULL) {
		cerr << "Unable to find DSSI node " << _node_path << endl;
	} else {
		string key = "dssi-configure--";
		key += _key;
		_engine.broadcaster()->send_metadata_update(_node_path, key, Atom(_val.c_str()));
	}
}


} // namespace Ingen

