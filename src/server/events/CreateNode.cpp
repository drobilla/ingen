/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include "raul/log.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"
#include "sord/sordmm.hpp"
#include "shared/LV2URIMap.hpp"
#include "CreateNode.hpp"
#include "Request.hpp"
#include "PatchImpl.hpp"
#include "NodeImpl.hpp"
#include "PluginImpl.hpp"
#include "Engine.hpp"
#include "PatchImpl.hpp"
#include "NodeFactory.hpp"
#include "ClientBroadcaster.hpp"
#include "EngineStore.hpp"
#include "PortImpl.hpp"
#include "Driver.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {
namespace Events {

CreateNode::CreateNode(
		Engine&                      engine,
		SharedPtr<Request>           request,
		SampleCount                  timestamp,
		const Path&                  path,
		const URI&                   plugin_uri,
		const Resource::Properties&  properties)
	: QueuedEvent(engine, request, timestamp)
	, _path(path)
	, _plugin_uri(plugin_uri)
	, _patch(NULL)
	, _plugin(NULL)
	, _node(NULL)
	, _compiled_patch(NULL)
	, _node_already_exists(false)
	, _polyphonic(false)
	, _properties(properties)
{
	const Resource::Properties::const_iterator p = properties.find(
			engine.world()->uris()->ingen_polyphonic);
	if (p != properties.end() && p->second.type() == Raul::Atom::BOOL
			&& p->second.get_bool())
		_polyphonic = true;
}

void
CreateNode::pre_process()
{
	if (_engine.engine_store()->find_object(_path) != NULL) {
		_node_already_exists = true;
		QueuedEvent::pre_process();
		return;
	}

	_patch  = _engine.engine_store()->find_patch(_path.parent());
	_plugin = _engine.node_factory()->plugin(_plugin_uri.str());

	if (_patch && _plugin) {

		_node = _plugin->instantiate(*_engine.buffer_factory(), _path.symbol(), _polyphonic, _patch, _engine);

		if (_node != NULL) {
			_node->properties().insert(_properties.begin(), _properties.end());
			_node->activate(*_engine.buffer_factory());

			// This can be done here because the audio thread doesn't touch the
			// node tree - just the process order array
			_patch->add_node(new PatchImpl::Nodes::Node(_node));
			_engine.engine_store()->add(_node);

			// FIXME: not really necessary to build process order since it's not connected,
			// just append to the list
			if (_patch->enabled())
				_compiled_patch = _patch->compile();
		}
	}

	if (!_node)
		_error = 1;

	QueuedEvent::pre_process();
}

void
CreateNode::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_node) {
		_engine.maid()->push(_patch->compiled_patch());
		_patch->compiled_patch(_compiled_patch);
	}
}

void
CreateNode::post_process()
{
	if (!_request)
		return;

	string msg;
	if (_node_already_exists) {
		msg = string("Could not create node - ").append(_path.str());// + " already exists.";
		_request->respond_error(msg);
	} else if (_patch == NULL) {
		msg = "Could not find patch '" + _path.parent().str() +"' to add node.";
		_request->respond_error(msg);
	} else if (_plugin == NULL) {
		msg = "Unable to load node ";
		msg += _path.str() + " (you're missing the plugin " + _plugin_uri.str() + ")";
		_request->respond_error(msg);
	} else if (_node == NULL) {
		msg = "Failed to instantiate plugin " + _plugin_uri.str();
		_request->respond_error(msg);
	} else {
		_request->respond_ok();
		_engine.broadcaster()->send_object(_node, true); // yes, send ports
	}
}

} // namespace Server
} // namespace Ingen
} // namespace Events
