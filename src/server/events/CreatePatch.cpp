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

#include "events/CreatePatch.hpp"
#include "Broadcaster.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "NodeImpl.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"

namespace Ingen {
namespace Server {
namespace Events {

CreatePatch::CreatePatch(Engine&                     engine,
                         Interface*                  client,
                         int32_t                     id,
                         SampleCount                 timestamp,
                         const Raul::Path&           path,
                         int                         poly,
                         const Resource::Properties& properties)
	: Event(engine, client, id, timestamp)
	, _path(path)
	, _patch(NULL)
	, _parent(NULL)
	, _compiled_patch(NULL)
	, _poly(poly)
	, _properties(properties)
{
}

void
CreatePatch::pre_process()
{
	if (_path.is_root() || _engine.engine_store()->find_object(_path) != NULL) {
		_status = EXISTS;
		Event::pre_process();
		return;
	}

	if (_poly < 1) {
		_status = INVALID_POLY;
		Event::pre_process();
		return;
	}

	const Raul::Path& path = (const Raul::Path&)_path;

	_parent = _engine.engine_store()->find_patch(path.parent());
	if (_parent == NULL) {
		_status = PARENT_NOT_FOUND;
		Event::pre_process();
		return;
	}

	uint32_t poly = 1;
	if (_parent != NULL && _poly > 1 && _poly == static_cast<int>(_parent->internal_poly()))
		poly = _poly;

	const Ingen::Shared::URIs& uris = _engine.world()->uris();

	_patch = new PatchImpl(_engine, path.symbol(), poly, _parent,
			_engine.driver()->sample_rate(), _poly);
	_patch->properties().insert(_properties.begin(), _properties.end());
	_patch->add_property(uris.rdf_type, uris.ingen_Patch);
	_patch->add_property(uris.rdf_type,
	                     Resource::Property(uris.ingen_Node, Resource::EXTERNAL));

	if (_parent != NULL) {
		_parent->add_node(new PatchImpl::Nodes::Node(_patch));

		if (_parent->enabled())
			_compiled_patch = _parent->compile();
	}

	_patch->activate(*_engine.buffer_factory());

	// Insert into EngineStore
	_engine.engine_store()->add(_patch);

	Event::pre_process();
}

void
CreatePatch::execute(ProcessContext& context)
{
	Event::execute(context);

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
	respond(_status);
	if (!_status) {
		// Don't send ports/nodes that have been added since prepare()
		// (otherwise they would be sent twice)
		_engine.broadcaster()->send_object(_patch, false);
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
