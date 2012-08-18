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
#include "Engine.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {
namespace Events {

CreateBlock::CreateBlock(Engine&                     engine,
                         SharedPtr<Interface>        client,
                         int32_t                     id,
                         SampleCount                 timestamp,
                         const Raul::Path&           path,
                         const Resource::Properties& properties)
	: Event(engine, client, id, timestamp)
	, _path(path)
	, _properties(properties)
	, _patch(NULL)
	, _block(NULL)
	, _compiled_patch(NULL)
{}

bool
CreateBlock::pre_process()
{
	Ingen::URIs& uris = _engine.world()->uris();

	typedef Resource::Properties::const_iterator iterator;

	if (_path.is_root()) {
		return Event::pre_process_done(BAD_URI, _path);
	}
		
	std::string plugin_uri_str;
	const iterator t = _properties.find(uris.ingen_prototype);
	if (t != _properties.end() && t->second.type() == uris.forge.URI) {
		plugin_uri_str = t->second.get_uri();
	} else {
		return Event::pre_process_done(BAD_REQUEST);
	}

	if (_engine.store()->get(_path)) {
		return Event::pre_process_done(EXISTS, _path);
	}

	_patch = dynamic_cast<PatchImpl*>(_engine.store()->get(_path.parent()));
	if (!_patch) {
		return Event::pre_process_done(PARENT_NOT_FOUND, _path.parent());
	}

	const Raul::URI plugin_uri(plugin_uri_str);
	PluginImpl* plugin = _engine.block_factory()->plugin(plugin_uri);
	if (!plugin) {
		return Event::pre_process_done(PLUGIN_NOT_FOUND, Raul::URI(plugin_uri));
	}

	const iterator p = _properties.find(uris.ingen_polyphonic);
	const bool polyphonic = (
		p != _properties.end() &&
		p->second.type() == _engine.world()->forge().Bool &&
		p->second.get_bool());

	if (!(_block = plugin->instantiate(*_engine.buffer_factory(),
	                                  Raul::Symbol(_path.symbol()),
	                                  polyphonic,
	                                  _patch,
	                                  _engine))) {
		return Event::pre_process_done(CREATION_FAILED, _path);
	}

	_block->properties().insert(_properties.begin(), _properties.end());
	_block->activate(*_engine.buffer_factory());

	// Add block to the store and the patch's pre-processor only block list
	_patch->add_block(*_block);
	_engine.store()->add(_block);

	/* Compile patch with new block added for insertion in audio thread
	   TODO: Since the block is not connected at this point, a full compilation
	   could be avoided and the block simply appended. */
	if (_patch->enabled()) {
		_compiled_patch = _patch->compile();
	}

	_update.push_back(make_pair(_block->uri(), _block->properties()));
	for (uint32_t i = 0; i < _block->num_ports(); ++i) {
		const PortImpl*      port   = _block->port_impl(i);
		Resource::Properties pprops = port->properties();
		pprops.erase(uris.ingen_value);
		pprops.insert(std::make_pair(uris.ingen_value, port->value()));
		_update.push_back(std::make_pair(port->uri(), pprops));
	}

	return Event::pre_process_done(SUCCESS);
}

void
CreateBlock::execute(ProcessContext& context)
{
	if (_block) {
		_engine.maid()->dispose(_patch->compiled_patch());
		_patch->compiled_patch(_compiled_patch);
	}
}

void
CreateBlock::post_process()
{
	if (!respond()) {
		for (Update::const_iterator i = _update.begin(); i != _update.end(); ++i) {
			_engine.broadcaster()->put(i->first, i->second);
		}
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
