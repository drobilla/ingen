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

#include "ClearPatchEvent.h"
#include "Responder.h"
#include "Ingen.h"
#include "Patch.h"
#include "ClientBroadcaster.h"
#include "util.h"
#include "ObjectStore.h"
#include "Port.h"
#include "Maid.h"
#include "Node.h"
#include "Connection.h"
#include "QueuedEventSource.h"

namespace Ingen {


ClearPatchEvent::ClearPatchEvent(CountedPtr<Responder> responder, SampleCount timestamp, const string& patch_path)
: QueuedEvent(responder, true),
  m_patch_path(patch_path),
  m_patch(NULL),
  m_process(false)
{
}


void
ClearPatchEvent::pre_process()
{
	m_patch = Ingen::instance().object_store()->find_patch(m_patch_path);
	
	if (m_patch != NULL) {
	
		m_process = m_patch->process();

		for (List<Node*>::const_iterator i = m_patch->nodes().begin(); i != m_patch->nodes().end(); ++i)
			(*i)->remove_from_store();
	}

	QueuedEvent::pre_process();
}


void
ClearPatchEvent::execute(SampleCount offset)
{
	if (m_patch != NULL) {
		m_patch->process(false);
		
		for (List<Node*>::const_iterator i = m_patch->nodes().begin(); i != m_patch->nodes().end(); ++i)
			(*i)->remove_from_patch();

		if (m_patch->process_order() != NULL) {
			Ingen::instance().maid()->push(m_patch->process_order());
			m_patch->process_order(NULL);
		}
	}
	
	QueuedEvent::execute(offset);
}


void
ClearPatchEvent::post_process()
{	
	if (m_patch != NULL) {
		// Delete all nodes
		for (List<Node*>::iterator i = m_patch->nodes().begin(); i != m_patch->nodes().end(); ++i) {
			(*i)->deactivate();
			delete *i;
		}
		m_patch->nodes().clear();

		// Delete all connections
		for (List<Connection*>::iterator i = m_patch->connections().begin(); i != m_patch->connections().end(); ++i)
			delete *i;
		m_patch->connections().clear();
		
		// Restore patch's run state
		m_patch->process(m_process);

		// Make sure everything's sane
		assert(m_patch->nodes().size() == 0);
		assert(m_patch->connections().size() == 0);
		
		// Reply
		_responder->respond_ok();
		Ingen::instance().client_broadcaster()->send_patch_cleared(m_patch_path);
	} else {
		_responder->respond_error(string("Patch ") + m_patch_path + " not found");
	}
	
	_source->unblock();
}


} // namespace Ingen

