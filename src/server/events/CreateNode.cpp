/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/shared/URIs.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"
#include "raul/log.hpp"
#include "sord/sordmm.hpp"

#include "CreateNode.hpp"
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

CreateNode::CreateNode(Engine&                     engine,
                       Interface*                  client,
                       int32_t                     id,
                       SampleCount                 timestamp,
                       const Path&                 path,
                       const URI&                  plugin_uri,
                       const Resource::Properties& properties)
	: Event(engine, client, id, timestamp)
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
	if (p != properties.end() && p->second.type() == engine.world()->forge().Bool
			&& p->second.get_bool())
		_polyphonic = true;
}

void
CreateNode::pre_process()
{
	if (_engine.engine_store()->find_object(_path) != NULL) {
		_node_already_exists = true;
		Event::pre_process();
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

	if (!_node) {
		_status = FAILURE;
	}

	Event::pre_process();
}

void
CreateNode::execute(ProcessContext& context)
{
	Event::execute(context);

	if (_node) {
		_engine.maid()->push(_patch->compiled_patch());
		_patch->compiled_patch(_compiled_patch);
	}
}

void
CreateNode::post_process()
{
	if (_node_already_exists) {
		respond(EXISTS);
	} else if (!_patch) {
		respond(PARENT_NOT_FOUND);
	} else if (!_plugin) {
		respond(PLUGIN_NOT_FOUND);
	} else if (!_node) {
		respond(FAILURE);
	} else {
		respond(SUCCESS);
		_engine.broadcaster()->send_object(_node, true); // yes, send ports
	}
}

} // namespace Server
} // namespace Ingen
} // namespace Events

