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

#include "AddNodeEvent.h"
#include "Responder.h"
#include "Patch.h"
#include "Node.h"
#include "Tree.h"
#include "Plugin.h"
#include "Engine.h"
#include "Patch.h"
#include "NodeFactory.h"
#include "ClientBroadcaster.h"
#include "Maid.h"
#include "raul/Path.h"
#include "ObjectStore.h"
#include "raul/Path.h"
#include "Port.h"

namespace Ingen {


AddNodeEvent::AddNodeEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& path,
		const string& plugin_uri, bool poly)
: QueuedEvent(engine, responder, timestamp),
  m_path(path),
  m_plugin_uri(plugin_uri),
  m_poly(poly),
  m_patch(NULL),
  m_node(NULL),
  m_process_order(NULL),
  m_node_already_exists(false)
{
}


/** DEPRECATED: Construct from type, library name, and plugin label.
 *
 * Do not use.
 */
AddNodeEvent::AddNodeEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& path,
		const string& plugin_type, const string& plugin_lib, const string& plugin_label, bool poly)
: QueuedEvent(engine, responder, timestamp),
  m_path(path),
  m_plugin_type(plugin_type),
  m_plugin_lib(plugin_lib),
  m_plugin_label(plugin_label),
  m_poly(poly),
  m_patch(NULL),
  m_node(NULL),
  m_process_order(NULL),
  m_node_already_exists(false)
{
}


void
AddNodeEvent::pre_process()
{
	if (_engine.object_store()->find(m_path) != NULL) {
		m_node_already_exists = true;
		QueuedEvent::pre_process();
		return;
	}

	m_patch = _engine.object_store()->find_patch(m_path.parent());

	const Plugin* plugin = (m_plugin_uri != "")
		? _engine.node_factory()->plugin(m_plugin_uri)
		: _engine.node_factory()->plugin(m_plugin_type, m_plugin_lib, m_plugin_label);

	if (m_patch && plugin) {
		if (m_poly)
			m_node = _engine.node_factory()->load_plugin(plugin, m_path.name(), m_patch->internal_poly(), m_patch);
		else
			m_node = _engine.node_factory()->load_plugin(plugin, m_path.name(), 1, m_patch);
		
		if (m_node != NULL) {
			m_node->activate();
		
			// This can be done here because the audio thread doesn't touch the
			// node tree - just the process order array
			m_patch->add_node(new ListNode<Node*>(m_node));
			m_node->add_to_store(_engine.object_store());
			
			// FIXME: not really necessary to build process order since it's not connected,
			// just append to the list
			if (m_patch->enabled())
				m_process_order = m_patch->build_process_order();
		}
	}
	QueuedEvent::pre_process();
}


void
AddNodeEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);

	if (m_node != NULL) {
		if (m_patch->process_order() != NULL)
			_engine.maid()->push(m_patch->process_order());
		m_patch->process_order(m_process_order);
	}
}


void
AddNodeEvent::post_process()
{
	string msg;
	if (m_node_already_exists) {
		msg = string("Could not create node - ").append(m_path);// + " already exists.";
		_responder->respond_error(msg);
	} else if (m_patch == NULL) {
		msg = "Could not find patch '" + m_path.parent() +"' for add_node.";
		_responder->respond_error(msg);
	} else if (m_node == NULL) {
		msg = "Unable to load node ";
		msg.append(m_path).append(" (you're missing the plugin \"").append(
			m_plugin_uri);
		_responder->respond_error(msg);
	} else {
		_responder->respond_ok();
		_engine.broadcaster()->send_node(m_node, true); // yes, send ports
	}
}


} // namespace Ingen

