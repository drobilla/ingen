/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "CreateBlock.hpp"

#include "BlockFactory.hpp"
#include "BlockImpl.hpp"
#include "Broadcaster.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "LV2Block.hpp"
#include "PluginImpl.hpp"
#include "PreProcessContext.hpp"
#include "types.hpp"

#include "ingen/FilePath.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Node.hpp"
#include "ingen/Properties.hpp"
#include "ingen/Resource.hpp"
#include "ingen/Status.hpp"
#include "ingen/Store.hpp"
#include "ingen/URI.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "ingen/paths.hpp"
#include "lilv/lilv.h"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"
#include "raul/Symbol.hpp"

#include <memory>
#include <utility>

namespace ingen {
namespace server {

class RunContext;

namespace events {

CreateBlock::CreateBlock(Engine&                           engine,
                         const std::shared_ptr<Interface>& client,
                         int32_t                           id,
                         SampleCount                       timestamp,
                         const Raul::Path&                 path,
                         Properties&                       properties)
    : Event(engine, client, id, timestamp)
    , _path(path)
    , _properties(properties)
    , _graph(nullptr)
    , _block(nullptr)
{}

bool
CreateBlock::pre_process(PreProcessContext& ctx)
{
	using iterator = Properties::const_iterator;

	const ingen::URIs&           uris  = _engine.world().uris();
	const std::shared_ptr<Store> store = _engine.store();

	// Check sanity of target path
	if (_path.is_root()) {
		return Event::pre_process_done(Status::BAD_URI, _path);
	} else if (store->get(_path)) {
		return Event::pre_process_done(Status::EXISTS, _path);
	} else if (!(_graph = dynamic_cast<GraphImpl*>(store->get(_path.parent())))) {
		return Event::pre_process_done(Status::PARENT_NOT_FOUND, _path.parent());
	}

	// Map old ingen:prototype to new lv2:prototype
	auto range = _properties.equal_range(uris.ingen_prototype);
	for (auto i = range.first; i != range.second;) {
		const auto value = i->second;
		auto       next  = i;
		next = _properties.erase(i);
		_properties.emplace(uris.lv2_prototype, value);
		i = next;
	}

	// Get prototype
	iterator t = _properties.find(uris.lv2_prototype);
	if (t == _properties.end() || !uris.forge.is_uri(t->second)) {
		// Missing/invalid prototype
		return Event::pre_process_done(Status::BAD_REQUEST);
	}

	const URI prototype(uris.forge.str(t->second, false));

	// Find polyphony
	const iterator p          = _properties.find(uris.ingen_polyphonic);
	const bool     polyphonic = (p != _properties.end() &&
	                             p->second.type() == uris.forge.Bool &&
	                             p->second.get<int32_t>());

	// Find and instantiate/duplicate prototype (plugin/existing node)
	if (uri_is_path(prototype)) {
		// Prototype is an existing block
		BlockImpl* const ancestor = dynamic_cast<BlockImpl*>(
			store->get(uri_to_path(prototype)));
		if (!ancestor) {
			return Event::pre_process_done(Status::PROTOTYPE_NOT_FOUND, prototype);
		} else if (!(_block = ancestor->duplicate(
			             _engine, Raul::Symbol(_path.symbol()), _graph))) {
			return Event::pre_process_done(Status::CREATION_FAILED, _path);
		}

		/* Replace prototype with the ancestor's.  This is less informative,
		   but the client expects an actual LV2 plugin as prototype. */
		_properties.erase(uris.ingen_prototype);
		_properties.erase(uris.lv2_prototype);
		_properties.emplace(uris.lv2_prototype,
		                    uris.forge.make_urid(ancestor->plugin()->uri()));
	} else {
		// Prototype is a plugin
		PluginImpl* const plugin = _engine.block_factory()->plugin(prototype);
		if (!plugin) {
			return Event::pre_process_done(Status::PROTOTYPE_NOT_FOUND, prototype);
		}

		// Load state from directory if given in properties
		LilvState* state = nullptr;
		auto s = _properties.find(uris.state_state);
		if (s != _properties.end() && s->second.type() == uris.forge.Path) {
			state = LV2Block::load_state(
				_engine.world(), FilePath(s->second.ptr<char>()));
		}

		// Instantiate plugin
		if (!(_block = plugin->instantiate(*_engine.buffer_factory(),
		                                   Raul::Symbol(_path.symbol()),
		                                   polyphonic,
		                                   _graph,
		                                   _engine,
		                                   state))) {
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
	_compiled_graph = ctx.maybe_compile(*_engine.maid(), *_graph);

	_update.put_block(_block);

	return Event::pre_process_done(Status::SUCCESS);
}

void
CreateBlock::execute(RunContext&)
{
	if (_status == Status::SUCCESS && _compiled_graph) {
		_graph->set_compiled_graph(std::move(_compiled_graph));
	}
}

void
CreateBlock::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS) {
		_update.send(*_engine.broadcaster());
	}
}

void
CreateBlock::undo(Interface& target)
{
	target.del(_block->uri());
}

} // namespace events
} // namespace server
} // namespace ingen
