/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "RenameEvent.h"
#include "Responder.h"
#include "Patch.h"
#include "Node.h"
#include "Tree.h"
#include "Om.h"
#include "OmApp.h"
#include "ClientBroadcaster.h"
#include "util/Path.h"
#include "ObjectStore.h"

namespace Om {


RenameEvent::RenameEvent(CountedPtr<Responder> responder, const string& path, const string& name)
: QueuedEvent(responder),
  m_old_path(path),
  m_name(name),
  m_new_path(m_old_path.parent().base_path() + name),
  m_parent_patch(NULL),
  m_store_treenode(NULL),
  m_error(NO_ERROR)
{
	/*
	if (m_old_path.parent() == "/")
		m_new_path = string("/") + m_name;
	else
		m_new_path = m_old_path.parent() +"/"+ m_name;*/
}


RenameEvent::~RenameEvent()
{
}


void
RenameEvent::pre_process()
{
	if (m_name.find("/") != string::npos) {
		m_error = INVALID_NAME;
		QueuedEvent::pre_process();
		return;
	}

	if (om->object_store()->find(m_new_path)) {
		m_error = OBJECT_EXISTS;
		QueuedEvent::pre_process();
		return;
	}
	
	GraphObject* obj = om->object_store()->find(m_old_path);

	if (obj == NULL) {
		m_error = OBJECT_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}
	
	// Renaming only works for Nodes and Patches (which are Nodes)
	/*if (obj->as_node() == NULL) {
		m_error = OBJECT_NOT_RENAMABLE;
		QueuedEvent::pre_process();
		return;
	}*/
	
	if (obj != NULL) {
		obj->set_path(m_new_path);
		assert(obj->path() == m_new_path);
	}
	
	QueuedEvent::pre_process();
}


void
RenameEvent::execute(samplecount offset)
{
	//cout << "Executing rename event...";
	QueuedEvent::execute(offset);
}


void
RenameEvent::post_process()
{
	string msg = "Unable to rename object - ";
	
	if (m_error == NO_ERROR) {
		m_responder->respond_ok();
		om->client_broadcaster()->send_rename(m_old_path, m_new_path);
	} else {
		if (m_error == OBJECT_EXISTS)
			msg.append("Object already exists at ").append(m_new_path);
		else if (m_error == OBJECT_NOT_FOUND)
			msg.append("Could not find object ").append(m_old_path);
		else if (m_error == OBJECT_NOT_RENAMABLE)
			msg.append(m_old_path).append(" is not renamable");
		else if (m_error == INVALID_NAME)
			msg.append(m_name).append(" is not a valid name");

		m_responder->respond_error(msg);
	}
}


} // namespace Om
