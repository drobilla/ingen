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

#include "DSSIProgramEvent.hpp"
#include <cstdio>
#include <iostream>
#include "Engine.hpp"
#include "Node.hpp"
#include "ClientBroadcaster.hpp"
#include "Plugin.hpp"
#include "ObjectStore.hpp"
using std::cout; using std::cerr; using std::endl;


namespace Ingen {


DSSIProgramEvent::DSSIProgramEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& node_path, int bank, int program)
: QueuedEvent(engine, responder, timestamp),
  _node_path(node_path),
  _bank(bank),
  _program(program),
  _node(NULL)
{
}


void
DSSIProgramEvent::pre_process()
{
	Node* node = _engine.object_store()->find_node(_node_path);

	if (node != NULL && node->plugin()->type() == Plugin::DSSI)
		_node = (DSSINode*)node;

	QueuedEvent::pre_process();
}

	
void
DSSIProgramEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_node != NULL)
		_node->program(_bank, _program);
}


void
DSSIProgramEvent::post_process()
{
	if (_node == NULL) {
		cerr << "Unable to find DSSI node " << _node_path << endl;
	} else {
		// sends program as metadata in the form bank/program
		char* temp_buf = new char[16];
		snprintf(temp_buf, 16, "%d/%d", _bank, _program);
		_engine.broadcaster()->send_metadata_update(_node_path, "dssi-program", temp_buf);
	}
}


} // namespace Ingen

