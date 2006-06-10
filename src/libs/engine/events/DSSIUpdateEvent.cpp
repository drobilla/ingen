/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "DSSIUpdateEvent.h"
#include <iostream>
#include "Node.h"
#include "ObjectStore.h"
#include "Om.h"
#include "OmApp.h"
#include "DSSIPlugin.h"
#include "Plugin.h"

using std::cerr; using std::endl;

namespace Om {


DSSIUpdateEvent::DSSIUpdateEvent(CountedPtr<Responder> responder, const string& path, const string& url)
: QueuedEvent(responder),
  m_path(path),
  m_url(url),
  m_node(NULL)
{
}


void
DSSIUpdateEvent::pre_process()
{
	Node* node = om->object_store()->find_node(m_path);

	if (node == NULL || node->plugin()->type() != Plugin::DSSI) {
		m_node = NULL;
		QueuedEvent::pre_process();
		return;
	} else {
		m_node = (DSSIPlugin*)node;
	}
	
	QueuedEvent::pre_process();
}


void
DSSIUpdateEvent::execute(samplecount offset)
{
	if (m_node != NULL) {
		m_node->set_ui_url(m_url);
	}
	
	QueuedEvent::execute(offset);
}


void
DSSIUpdateEvent::post_process()
{
	cerr << "DSSI update event: " << m_url << endl;

	if (m_node != NULL) {
		m_node->send_update();
	}
}


} // namespace Om

