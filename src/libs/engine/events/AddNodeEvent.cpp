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

#include <raul/Maid.hpp>
#include <raul/Path.hpp>
#include <raul/Path.hpp>
#include "AddNodeEvent.hpp"
#include "Responder.hpp"
#include "Patch.hpp"
#include "Node.hpp"
#include "Tree.hpp"
#include "Plugin.hpp"
#include "Engine.hpp"
#include "Patch.hpp"
#include "NodeFactory.hpp"
#include "ClientBroadcaster.hpp"
#include "ObjectStore.hpp"
#include "Port.hpp"

namespace Ingen {


AddNodeEvent::AddNodeEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& path,
		const string& plugin_uri, bool poly)
: QueuedEvent(engine, responder, timestamp),
  _path(path),
  _plugin_uri(plugin_uri),
  _poly(poly),
  _patch(NULL),
  _node(NULL),
  _process_order(NULL),
  _node_already_exists(false)
{
}


/** DEPRECATED: Construct from type, library name, and plugin label.
 *
 * Do not use.
 */
AddNodeEvent::AddNodeEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& path,
		const string& plugin_type, const string& plugin_lib, const string& plugin_label, bool poly)
: QueuedEvent(engine, responder, timestamp),
  _path(path),
  _plugin_type(plugin_type),
  _plugin_lib(plugin_lib),
  _plugin_label(plugin_label),
  _poly(poly),
  _patch(NULL),
  _node(NULL),
  _process_order(NULL),
  _node_already_exists(false)
{
}


void
AddNodeEvent::pre_process()
{
	if (_engine.object_store()->find_object(_path) != NULL) {
		_node_already_exists = true;
		QueuedEvent::pre_process();
		return;
	}

	_patch = _engine.object_store()->find_patch(_path.parent());

	const Plugin* plugin = (_plugin_uri != "")
		? _engine.node_factory()->plugin(_plugin_uri)
		: _engine.node_factory()->plugin(_plugin_type, _plugin_lib, _plugin_label);

	if (_patch && plugin) {
		if (_poly)
			_node = _engine.node_factory()->load_plugin(plugin, _path.name(), _patch->internal_poly(), _patch);
		else
			_node = _engine.node_factory()->load_plugin(plugin, _path.name(), 1, _patch);
		
		if (_node != NULL) {
			_node->activate();
		
			// This can be done here because the audio thread doesn't touch the
			// node tree - just the process order array
			_patch->add_node(new Raul::ListNode<Node*>(_node));
			//_node->add_to_store(_engine.object_store());
			_engine.object_store()->add(_node);
			
			// FIXME: not really necessary to build process order since it's not connected,
			// just append to the list
			if (_patch->enabled())
				_process_order = _patch->build_process_order();
		}
	}
	QueuedEvent::pre_process();
}


void
AddNodeEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);

	if (_node != NULL) {
		if (_patch->process_order() != NULL)
			_engine.maid()->push(_patch->process_order());
		_patch->process_order(_process_order);
	}
}


void
AddNodeEvent::post_process()
{
	string msg;
	if (_node_already_exists) {
		msg = string("Could not create node - ").append(_path);// + " already exists.";
		_responder->respond_error(msg);
	} else if (_patch == NULL) {
		msg = "Could not find patch '" + _path.parent() +"' for add_node.";
		_responder->respond_error(msg);
	} else if (_node == NULL) {
		msg = "Unable to load node ";
		msg.append(_path).append(" (you're missing the plugin \"").append(
			_plugin_uri);
		_responder->respond_error(msg);
	} else {
		_responder->respond_ok();
		_engine.broadcaster()->send_node(_node, true); // yes, send ports
	}
}


} // namespace Ingen

