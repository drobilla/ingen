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

#include "ingen/shared/URIs.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"
#include "raul/log.hpp"
#include "sord/sordmm.hpp"

#include "Broadcaster.hpp"
#include "CreateNode.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "NodeFactory.hpp"
#include "NodeImpl.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {
namespace Events {

CreateNode::CreateNode(Engine&                     engine,
                       Interface*                  client,
                       int32_t                     id,
                       SampleCount                 timestamp,
                       const Raul::Path&           path,
                       const Raul::URI&            plugin_uri,
                       const Resource::Properties& properties)
	: Event(engine, client, id, timestamp)
	, _path(path)
	, _plugin_uri(plugin_uri)
	, _properties(properties)
	, _patch(NULL)
	, _node(NULL)
	, _compiled_patch(NULL)
{}

void
CreateNode::pre_process()
{
	Ingen::Shared::URIs& uris = _engine.world()->uris();

	if (_engine.engine_store()->find_object(_path)) {
		_status = EXISTS;
		Event::pre_process();
		return;
	}

	if (!(_patch = _engine.engine_store()->find_patch(_path.parent()))) {
		_status = PARENT_NOT_FOUND;
		Event::pre_process();
		return;
	}

	PluginImpl* plugin = _engine.node_factory()->plugin(_plugin_uri.str());
	if (!plugin) {
		_status = PLUGIN_NOT_FOUND;
		Event::pre_process();
		return;
	}

	const Resource::Properties::const_iterator p = _properties.find(
		_engine.world()->uris().ingen_polyphonic);
	const bool polyphonic = (
		p != _properties.end() &&
		p->second.type() == _engine.world()->forge().Bool &&
		p->second.get_bool());
	
	if (!(_node = plugin->instantiate(*_engine.buffer_factory(),
	                                  _path.symbol(),
	                                  polyphonic,
	                                  _patch,
	                                  _engine))) {
		_status = CREATION_FAILED;
		Event::pre_process();
		return;
	}

	_node->properties().insert(_properties.begin(), _properties.end());
	_node->activate(*_engine.buffer_factory());

	// Add node to the store and the  patch's pre-processor only node list
	_patch->add_node(new PatchImpl::Nodes::Node(_node));
	_engine.engine_store()->add(_node);

	/* Compile patch with new node added for insertion in audio thread
	   TODO: Since the node is not connected at this point, a full compilation
	   could be avoided and the node simply appended. */
	if (_patch->enabled()) {
		_compiled_patch = _patch->compile();
		assert(_compiled_patch);
	}

	_update.push_back(make_pair(_node->path(), _node->properties()));
	for (uint32_t i = 0; i < _node->num_ports(); ++i) {
		const PortImpl*      port   = _node->port_impl(i);
		Resource::Properties pprops = port->properties();
		pprops.erase(uris.ingen_value);
		pprops.insert(std::make_pair(uris.ingen_value, port->value()));
		_update.push_back(std::make_pair(port->path(), pprops));
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
	if (_status) {
		respond(_status);
	} else {
		respond(SUCCESS);
		for (Update::const_iterator i = _update.begin(); i != _update.end(); ++i) {
			_engine.broadcaster()->put(i->first, i->second);
		}
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
