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

#include "DestroyEvent.h"
#include "Responder.h"
#include "Engine.h"
#include "Patch.h"
#include "Tree.h"
#include "Node.h"
#include "Plugin.h"
#include "InternalNode.h"
#include "DisconnectNodeEvent.h"
#include "DisconnectPortEvent.h"
#include "ClientBroadcaster.h"
#include "Maid.h"
#include "ObjectStore.h"
#include "util/Path.h"
#include "QueuedEventSource.h"
#include "Port.h"

namespace Ingen {


DestroyEvent::DestroyEvent(Engine& engine, CountedPtr<Responder> responder, FrameTime time, QueuedEventSource* source, const string& path, bool block)
: QueuedEvent(engine, responder, time, source, source),
  m_path(path),
  m_node(NULL),
  m_patch_listnode(NULL),
  m_store_treenode(NULL),
  m_process_order(NULL),
  m_disconnect_event(NULL)
{
	assert(_source);
}


DestroyEvent::DestroyEvent(Engine& engine, CountedPtr<Responder> responder, FrameTime time, QueuedEventSource* source, Node* node, bool block)
: QueuedEvent(engine, responder, block, source),
  m_path(node->path()),
  m_node(node),
  m_patch_listnode(NULL),
  m_store_treenode(NULL),
  m_process_order(NULL),
  m_disconnect_event(NULL)
{
}


DestroyEvent::~DestroyEvent()
{
	delete m_disconnect_event;
}


void
DestroyEvent::pre_process()
{
	if (m_node == NULL)
		m_node = _engine.object_store()->find_node(m_path);

	if (m_node != NULL && m_path != "/") {
		assert(m_node->parent_patch() != NULL);
		m_patch_listnode = m_node->parent_patch()->remove_node(m_path.name());
		if (m_patch_listnode != NULL) {
			assert(m_patch_listnode->elem() == m_node);
			
			m_node->remove_from_store();

			if (m_node->providers()->size() != 0 || m_node->dependants()->size() != 0) {
				m_disconnect_event = new DisconnectNodeEvent(_engine, m_node);
				m_disconnect_event->pre_process();
			}
			
			if (m_node->parent_patch()->enabled()) {
				m_process_order = m_node->parent_patch()->build_process_order();
				// Remove node to be removed from the process order so it isn't executed by
				// Patch::run and can safely be destroyed
				//for (size_t i=0; i < m_process_order->size(); ++i)
				//	if (m_process_order->at(i) == m_node)
				//		m_process_order->at(i) = NULL; // ew, gap
				
#ifdef DEBUG
				// Be sure node is removed from process order, so it can be destroyed
				for (size_t i=0; i < m_process_order->size(); ++i)
					assert(m_process_order->at(i) != m_node);
#endif
			}
		}
	}

	QueuedEvent::pre_process();
}


void
DestroyEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);

	if (m_patch_listnode != NULL) {
		m_node->remove_from_patch();
		
		if (m_disconnect_event != NULL)
			m_disconnect_event->execute(nframes, start, end);
		
		if (m_node->parent_patch()->process_order() != NULL)
			_engine.maid()->push(m_node->parent_patch()->process_order());
		m_node->parent_patch()->process_order(m_process_order);
	}
}


void
DestroyEvent::post_process()
{
	if (_source)
		_source->unblock();
	
	if (m_node == NULL) {
		if (m_path == "/")
			_responder->respond_error("You can not destroy the root patch (/)");
		else
			_responder->respond_error("Could not find node to destroy");
	} else if (m_patch_listnode != NULL) {	
		m_node->deactivate();
		_responder->respond_ok();
		if (m_disconnect_event != NULL)
			m_disconnect_event->post_process();
		_engine.broadcaster()->send_destroyed(m_path);
		_engine.maid()->push(m_patch_listnode);
		_engine.maid()->push(m_node);
	} else {
		_responder->respond_error("Unable to destroy object");
	}
}


} // namespace Ingen
