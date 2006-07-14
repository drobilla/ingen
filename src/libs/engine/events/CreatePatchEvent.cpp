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

#include "CreatePatchEvent.h"
#include "Responder.h"
#include "Patch.h"
#include "Node.h"
#include "Tree.h"
#include "Plugin.h"
#include "Om.h"
#include "OmApp.h"
#include "Maid.h"
#include "ClientBroadcaster.h"
#include "AudioDriver.h"
#include "util/Path.h"
#include "ObjectStore.h"

namespace Om {


CreatePatchEvent::CreatePatchEvent(CountedPtr<Responder> responder, samplecount timestamp, const string& path, int poly)
: QueuedEvent(responder, timestamp),
  m_path(path),
  m_patch(NULL),
  m_parent(NULL),
  m_process_order(NULL),
  m_poly(poly),
  m_error(NO_ERROR)
{
}


void
CreatePatchEvent::pre_process()
{
	if (om->object_store()->find(m_path) != NULL) {
		m_error = OBJECT_EXISTS;
		QueuedEvent::pre_process();
		return;
	}

	if (m_poly < 1) {
		m_error = INVALID_POLY;
		QueuedEvent::pre_process();
		return;
	}
	
	if (m_path != "/") {
		m_parent = om->object_store()->find_patch(m_path.parent());
		if (m_parent == NULL) {
			m_error = PARENT_NOT_FOUND;
			QueuedEvent::pre_process();
			return;
		}
	}
	
	size_t poly = 1;
	if (m_parent != NULL && m_poly > 1 && m_poly == static_cast<int>(m_parent->internal_poly()))
		poly = m_poly;
	
	m_patch = new Patch(m_path.name(), poly, m_parent, om->audio_driver()->sample_rate(), om->audio_driver()->buffer_size(), m_poly);
		
	if (m_parent != NULL) {
		m_parent->add_node(new ListNode<Node*>(m_patch));

		if (m_parent->process())
			m_process_order = m_parent->build_process_order();
	}
	
	m_patch->activate();
	
	// Insert into ObjectStore
	m_patch->add_to_store();
	
	QueuedEvent::pre_process();
}


void
CreatePatchEvent::execute(samplecount offset)
{
	QueuedEvent::execute(offset);

	if (m_patch != NULL) {
		if (m_parent == NULL) {
			assert(m_path == "/");
			assert(m_patch->parent_patch() == NULL);
			om->audio_driver()->set_root_patch(m_patch);
		} else {
			assert(m_parent != NULL);
			assert(m_path != "/");
			
			m_patch->add_to_patch();
			
			if (m_parent->process_order() != NULL)
				om->maid()->push(m_parent->process_order());
			m_parent->process_order(m_process_order);
		}
	}
}


void
CreatePatchEvent::post_process()
{
	if (_responder.get()) {
		if (m_error == NO_ERROR) {
			
			_responder->respond_ok();
			
			// Don't want to send nodes that have been added since prepare()
			//om->client_broadcaster()->send_node_creation_messages(m_patch);

			// Patches are always empty on creation, so this is fine
			om->client_broadcaster()->send_patch(m_patch);
			
		} else if (m_error == OBJECT_EXISTS) {
			string msg = "Unable to create patch: ";
			msg += m_path += " already exists.";
			_responder->respond_error(msg);
		} else if (m_error == PARENT_NOT_FOUND) {
			string msg = "Unable to create patch: Parent ";
			msg += m_path.parent() += " not found.";
			_responder->respond_error(msg);
		} else if (m_error == INVALID_POLY) {
			string msg = "Unable to create patch ";
			msg.append(m_path).append(": ").append("Invalid polyphony respondered.");
			_responder->respond_error(msg);
		} else {
			_responder->respond_error("Unable to load patch.");
		}
	}
}


} // namespace Om

