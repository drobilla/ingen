/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#include "raul/Maid.hpp"
#include "raul/Path.hpp"
#include "shared/LV2URIMap.hpp"
#include "events/CreatePatch.hpp"
#include "Request.hpp"
#include "PatchImpl.hpp"
#include "NodeImpl.hpp"
#include "PluginImpl.hpp"
#include "Engine.hpp"
#include "ClientBroadcaster.hpp"
#include "Driver.hpp"
#include "EngineStore.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Events {

using namespace Shared;


CreatePatch::CreatePatch(
		Engine&                     engine,
		SharedPtr<Request>          request,
		SampleCount                 timestamp,
		const Raul::Path&           path,
		int                         poly,
		const Resource::Properties& properties)
	: QueuedEvent(engine, request, timestamp)
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
		_error = OBJECT_EXISTS;
		QueuedEvent::pre_process();
		return;
	}

	if (_poly < 1) {
		_error = INVALID_POLY;
		QueuedEvent::pre_process();
		return;
	}

	const Path& path = (const Path&)_path;

	_parent = _engine.engine_store()->find_patch(path.parent());
	if (_parent == NULL) {
		_error = PARENT_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}

	uint32_t poly = 1;
	if (_parent != NULL && _poly > 1 && _poly == static_cast<int>(_parent->internal_poly()))
		poly = _poly;

	const LV2URIMap& uris = *_engine.world()->uris().get();

	_patch = new PatchImpl(_engine, path.symbol(), poly, _parent,
			_engine.driver()->sample_rate(), _poly);
	_patch->meta().properties().insert(_properties.begin(), _properties.end());
	_patch->meta().set_property(uris.rdf_type, uris.ingen_Patch);
	_patch->set_property(uris.rdf_type, uris.ingen_Node);

	if (_parent != NULL) {
		_parent->add_node(new PatchImpl::Nodes::Node(_patch));

		if (_parent->enabled())
			_compiled_patch = _parent->compile();
	}

	_patch->activate(*_engine.buffer_factory());

	// Insert into EngineStore
	//_patch->add_to_store(_engine.engine_store());
	_engine.engine_store()->add(_patch);

	QueuedEvent::pre_process();
}


void
CreatePatch::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_patch) {
		if (!_parent) {
			assert(_path.is_root());
			assert(_patch->parent_patch() == NULL);
			_engine.driver()->set_root_patch(_patch);
		} else {
			assert(_parent);
			assert(!_path.is_root());
			_engine.maid()->push(_parent->compiled_patch());
			_parent->compiled_patch(_compiled_patch);
		}
	}
}


void
CreatePatch::post_process()
{
	string msg;
	if (_request) {
		switch (_error) {
		case NO_ERROR:
			_request->respond_ok();
			// Don't send ports/nodes that have been added since prepare()
			// (otherwise they would be sent twice)
			_engine.broadcaster()->send_object(_patch, false);
			break;
		case OBJECT_EXISTS:
			_request->respond_ok();
			/*string msg = "Unable to create patch: ";
			msg.append(_path).append(" already exists.");
			_request->respond_error(msg);*/
			break;
		case PARENT_NOT_FOUND:
			msg = "Unable to create patch: Parent ";
			msg.append(Path(_path).parent().str()).append(" not found.");
			_request->respond_error(msg);
			break;
		case INVALID_POLY:
			msg = "Unable to create patch ";
			msg.append(_path.str()).append(": ").append("Invalid polyphony requested.");
			_request->respond_error(msg);
			break;
		default:
			_request->respond_error("Unable to load patch.");
		}
	}
}


} // namespace Ingen
} // namespace Events

