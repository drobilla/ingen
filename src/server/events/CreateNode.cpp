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
                       SharedPtr<Interface>        client,
                       int32_t                     id,
                       SampleCount                 timestamp,
                       const Raul::Path&           path,
                       const Resource::Properties& properties)
	: Event(engine, client, id, timestamp)
	, _path(path)
	, _properties(properties)
	, _patch(NULL)
	, _node(NULL)
	, _compiled_patch(NULL)
{}

bool
CreateNode::pre_process()
{
	Ingen::Shared::URIs& uris = _engine.world()->uris();

	typedef Resource::Properties::const_iterator iterator;

	const iterator t = _properties.find(uris.ingen_prototype);
	if (t != _properties.end() && t->second.type() == uris.forge.URI) {
		_plugin_uri = t->second.get_uri();
	} else {
		return Event::pre_process_done(BAD_REQUEST);
	}

	if (_engine.engine_store()->find_object(_path)) {
		return Event::pre_process_done(EXISTS);
	}

	if (!(_patch = _engine.engine_store()->find_patch(_path.parent()))) {
		return Event::pre_process_done(PARENT_NOT_FOUND);
	}

	PluginImpl* plugin = _engine.node_factory()->plugin(_plugin_uri);
	if (!plugin) {
		return Event::pre_process_done(PLUGIN_NOT_FOUND);
	}

	const iterator p = _properties.find(uris.ingen_polyphonic);
	const bool polyphonic = (
		p != _properties.end() &&
		p->second.type() == _engine.world()->forge().Bool &&
		p->second.get_bool());

	if (!(_node = plugin->instantiate(*_engine.buffer_factory(),
	                                  _path.symbol(),
	                                  polyphonic,
	                                  _patch,
	                                  _engine))) {
		return Event::pre_process_done(CREATION_FAILED);
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
	}

	_update.push_back(make_pair(_node->path(), _node->properties()));
	for (uint32_t i = 0; i < _node->num_ports(); ++i) {
		const PortImpl*      port   = _node->port_impl(i);
		Resource::Properties pprops = port->properties();
		pprops.erase(uris.ingen_value);
		pprops.insert(std::make_pair(uris.ingen_value, port->value()));
		_update.push_back(std::make_pair(port->path(), pprops));
	}

	return Event::pre_process_done(SUCCESS);
}

void
CreateNode::execute(ProcessContext& context)
{
	if (_node) {
		_engine.maid()->push(_patch->compiled_patch());
		_patch->compiled_patch(_compiled_patch);
	}
}

void
CreateNode::post_process()
{
	respond(_status);
	if (!_status) {
		for (Update::const_iterator i = _update.begin(); i != _update.end(); ++i) {
			_engine.broadcaster()->put(i->first, i->second);
		}
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
