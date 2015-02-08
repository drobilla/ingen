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

#include "ingen/Store.hpp"
#include "ingen/URIs.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"

#include "BlockFactory.hpp"
#include "BlockImpl.hpp"
#include "Broadcaster.hpp"
#include "CreateBlock.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {
namespace Events {

CreateBlock::CreateBlock(Engine&                     engine,
                         SPtr<Interface>             client,
                         int32_t                     id,
                         SampleCount                 timestamp,
                         const Raul::Path&           path,
                         const Resource::Properties& properties)
	: Event(engine, client, id, timestamp)
	, _path(path)
	, _properties(properties)
	, _graph(NULL)
	, _block(NULL)
	, _compiled_graph(NULL)
{}

CreateBlock::~CreateBlock()
{
	delete _compiled_graph;
}

bool
CreateBlock::pre_process()
{
	typedef Resource::Properties::const_iterator iterator;

	const Ingen::URIs& uris  = _engine.world()->uris();
	const SPtr<Store>  store = _engine.store();

	// Check sanity of target path
	if (_path.is_root()) {
		return Event::pre_process_done(Status::BAD_URI, _path);
	} else if (store->get(_path)) {
		return Event::pre_process_done(Status::EXISTS, _path);
	} else if (!(_graph = dynamic_cast<GraphImpl*>(store->get(_path.parent())))) {
		return Event::pre_process_done(Status::PARENT_NOT_FOUND, _path.parent());
	}

	// Get prototype URI
	const iterator t = _properties.find(uris.ingen_prototype);
	if (t == _properties.end() || t->second.type() != uris.forge.URI) {
		return Event::pre_process_done(Status::BAD_REQUEST);
	}

	const Raul::URI prototype(t->second.ptr<char>());

	// Find polyphony
	const iterator p          = _properties.find(uris.ingen_polyphonic);
	const bool     polyphonic = (p != _properties.end() &&
	                             p->second.type() == uris.forge.Bool &&
	                             p->second.get<int32_t>());

	// Find and instantiate/duplicate prototype (plugin/existing node)
	if (Node::uri_is_path(prototype)) {
		// Prototype is an existing block
		BlockImpl* const ancestor = dynamic_cast<BlockImpl*>(
			store->get(Node::uri_to_path(prototype)));
		if (!ancestor) {
			return Event::pre_process_done(Status::PROTOTYPE_NOT_FOUND, prototype);
		} else if (!(_block = ancestor->duplicate(
			             _engine, Raul::Symbol(_path.symbol()), _graph))) {
			return Event::pre_process_done(Status::CREATION_FAILED, _path);
		}

		/* Replace prototype with the ancestor's.  This is less informative,
		   but the client expects an actual LV2 plugin as prototype. */
		_properties.erase(uris.ingen_prototype);
		_properties.insert(std::make_pair(uris.ingen_prototype,
		                                  uris.forge.alloc_uri(ancestor->plugin()->uri())));
	} else {
		// Prototype is a plugin
		PluginImpl* const plugin = _engine.block_factory()->plugin(prototype);
		if (!plugin) {
			return Event::pre_process_done(Status::PROTOTYPE_NOT_FOUND, prototype);
		} else if (!(_block = plugin->instantiate(*_engine.buffer_factory(),
		                                          Raul::Symbol(_path.symbol()),
		                                          polyphonic,
		                                          _graph,
		                                          _engine))) {
			return Event::pre_process_done(Status::CREATION_FAILED, _path);
		}
	}

	// Activate block
	_block->properties().insert(_properties.begin(), _properties.end());
	_block->activate(*_engine.buffer_factory());

	// Add block to the store and the graph's pre-processor only block list
	_graph->add_block(*_block);
	store->add(_block);

	/* Compile graph with new block added for insertion in audio thread
	   TODO: Since the block is not connected at this point, a full compilation
	   could be avoided and the block simply appended. */
	if (_graph->enabled()) {
		_compiled_graph = _graph->compile();
	}

	_update.put_block(_block);

	return Event::pre_process_done(Status::SUCCESS);
}

void
CreateBlock::execute(ProcessContext& context)
{
	if (_block) {
		_graph->set_compiled_graph(_compiled_graph);
		_compiled_graph = NULL;  // Graph takes ownership
	}
}

void
CreateBlock::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS) {
		_update.send(_engine.broadcaster());
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
