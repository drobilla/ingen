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

#include "DSSIProgramEvent.h"
#include <cstdio>
#include <iostream>
#include "Engine.h"
#include "Node.h"
#include "ClientBroadcaster.h"
#include "Plugin.h"
#include "ObjectStore.h"
using std::cout; using std::cerr; using std::endl;


namespace Ingen {


DSSIProgramEvent::DSSIProgramEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& node_path, int bank, int program)
: QueuedEvent(engine, responder, timestamp),
  m_node_path(node_path),
  m_bank(bank),
  m_program(program),
  m_node(NULL)
{
}


void
DSSIProgramEvent::pre_process()
{
	Node* node = _engine.object_store()->find_node(m_node_path);

	if (node != NULL && node->plugin()->type() == Plugin::DSSI)
		m_node = (DSSINode*)node;

	QueuedEvent::pre_process();
}

	
void
DSSIProgramEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);

	if (m_node != NULL)
		m_node->program(m_bank, m_program);
}


void
DSSIProgramEvent::post_process()
{
	if (m_node == NULL) {
		cerr << "Unable to find DSSI node " << m_node_path << endl;
	} else {
		// sends program as metadata in the form bank/program
		char* temp_buf = new char[16];
		snprintf(temp_buf, 16, "%d/%d", m_bank, m_program);
		_engine.broadcaster()->send_metadata_update(m_node_path, "dssi-program", temp_buf);
	}
}


} // namespace Ingen

