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

#include "ingen/URIs.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"

#include "events/CreatePatch.hpp"
#include "Broadcaster.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "PatchImpl.hpp"

namespace Ingen {
namespace Server {
namespace Events {

CreatePatch::CreatePatch(Engine&                     engine,
                         SharedPtr<Interface>        client,
                         int32_t                     id,
                         SampleCount                 timestamp,
                         const Raul::Path&           path,
                         const Resource::Properties& properties)
	: Event(engine, client, id, timestamp)
	, _path(path)
	, _properties(properties)
	, _patch(NULL)
	, _parent(NULL)
	, _compiled_patch(NULL)
{
}

bool
CreatePatch::pre_process()
{
	if (_path.is_root() || _engine.engine_store()->find_object(_path) != NULL) {
		return Event::pre_process_done(EXISTS, _path);
	}

	_parent = _engine.engine_store()->find_patch(_path.parent());
	if (!_parent) {
		return Event::pre_process_done(PARENT_NOT_FOUND, _path.parent());
	}

	const Ingen::URIs& uris = _engine.world()->uris();

	typedef Resource::Properties::const_iterator iterator;

	uint32_t ext_poly = 1;
	uint32_t int_poly = 1;
	iterator p        = _properties.find(uris.ingen_polyphony);
	if (p != _properties.end() && p->second.type() == uris.forge.Int) {
		int_poly = p->second.get_int32();
	}

	if (int_poly < 1 || int_poly > 128) {
		return Event::pre_process_done(INVALID_POLY, _path);
	}

	if (int_poly == _parent->internal_poly()) {
		ext_poly = int_poly;
	}

	const Raul::Symbol symbol((_path.is_root()) ? "root" : _path.symbol());
	_patch = new PatchImpl(_engine, symbol, ext_poly, _parent,
	                       _engine.driver()->sample_rate(), int_poly);
	_patch->properties().insert(_properties.begin(), _properties.end());
	_patch->add_property(uris.rdf_type, uris.ingen_Patch);
	_patch->add_property(uris.rdf_type,
	                     Resource::Property(uris.ingen_Node, Resource::EXTERNAL));

	_parent->add_node(new PatchImpl::Nodes::Node(_patch));
	if (_parent->enabled()) {
		_patch->enable();
		_compiled_patch = _parent->compile();
	}

	_patch->activate(*_engine.buffer_factory());

	// Insert into EngineStore
	_engine.engine_store()->add(_patch);

	_update = _patch->properties();

	return Event::pre_process_done(SUCCESS);
}

void
CreatePatch::execute(ProcessContext& context)
{
	if (_patch) {
		assert(_parent);
		assert(!_path.is_root());
		_engine.maid()->push(_parent->compiled_patch());
		_parent->compiled_patch(_compiled_patch);
	}
}

void
CreatePatch::post_process()
{
	if (!respond()) {
		_engine.broadcaster()->put(GraphObject::path_to_uri(_path), _update);
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
