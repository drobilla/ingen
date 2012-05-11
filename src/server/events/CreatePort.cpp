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

#include <utility>

#include "ingen/shared/URIMap.hpp"
#include "ingen/shared/URIs.hpp"
#include "raul/Array.hpp"
#include "raul/Atom.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"

#include "ClientBroadcaster.hpp"
#include "ControlBindings.hpp"
#include "CreatePort.hpp"
#include "Driver.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {
namespace Events {

CreatePort::CreatePort(Engine&                     engine,
                       Interface*                  client,
                       int32_t                     id,
                       SampleCount                 timestamp,
                       const Raul::Path&           path,
                       bool                        is_output,
                       const Resource::Properties& properties)
	: Event(engine, client, id, timestamp)
	, _path(path)
	, _port_type(PortType::UNKNOWN)
	, _buffer_type(0)
	, _patch(NULL)
	, _patch_port(NULL)
	, _ports_array(NULL)
	, _engine_port(NULL)
	, _properties(properties)
	, _is_output(is_output)
{
	const Ingen::Shared::URIs& uris = _engine.world()->uris();

	typedef Resource::Properties::const_iterator Iterator;
	typedef std::pair<Iterator, Iterator>        Range;

	const Range types = properties.equal_range(uris.rdf_type);
	for (Iterator i = types.first; i != types.second; ++i) {
		const Raul::Atom& type = i->second;
		if (type == uris.lv2_AudioPort) {
			_port_type = PortType::AUDIO;
		} else if (type == uris.lv2_ControlPort) {
			_port_type = PortType::CONTROL;
		} else if (type == uris.lv2_CVPort) {
			_port_type = PortType::CV;
		} else if (type == uris.atom_AtomPort) {
			_port_type = PortType::ATOM;
		}
	}

	const Range buffer_types = properties.equal_range(uris.atom_bufferType);
	for (Iterator i = buffer_types.first; i != buffer_types.second; ++i) {
		if (i->second.type() == _engine.world()->forge().URI) {
			_buffer_type = _engine.world()->uri_map().map_uri(i->second.get_uri());
		}
	}

	if (_port_type == PortType::UNKNOWN) {
		_status = UNKNOWN_TYPE;
	}
}

void
CreatePort::pre_process()
{
	if (_status == UNKNOWN_TYPE || _engine.engine_store()->find_object(_path)) {
		Event::pre_process();
		return;
	}

	_patch = _engine.engine_store()->find_patch(_path.parent());

	const Ingen::Shared::URIs& uris = _engine.world()->uris();

	if (_patch != NULL) {
		assert(_patch->path() == _path.parent());

		size_t buffer_size = _engine.buffer_factory()->default_buffer_size(_buffer_type);

		const uint32_t old_num_ports = (_patch->external_ports())
			? _patch->external_ports()->size()
			: 0;

		Resource::Properties::const_iterator index_i = _properties.find(uris.lv2_index);
		if (index_i == _properties.end()) {
			index_i = _properties.insert(
				std::make_pair(uris.lv2_index,
				               _engine.world()->forge().make(int32_t(old_num_ports))));
		} else if (index_i->second.type() != uris.forge.Int
				|| index_i->second.get_int32() != static_cast<int32_t>(old_num_ports)) {
			Event::pre_process();
			_status = BAD_INDEX;
			return;
		}

		Resource::Properties::const_iterator poly_i = _properties.find(uris.ingen_polyphonic);
		bool polyphonic = (poly_i != _properties.end() && poly_i->second.type() == uris.forge.Bool
				&& poly_i->second.get_bool());

		_patch_port = _patch->create_port(*_engine.buffer_factory(), _path.symbol(),
		                                  _port_type, _buffer_type, buffer_size,
		                                  _is_output, polyphonic);

		_patch_port->properties().insert(_properties.begin(), _properties.end());

		assert(index_i->second == _engine.world()->forge().make((int)_patch_port->index()));

		if (_patch_port) {

			if (_is_output)
				_patch->add_output(new Raul::List<PortImpl*>::Node(_patch_port));
			else
				_patch->add_input(new Raul::List<PortImpl*>::Node(_patch_port));

			if (_patch->external_ports())
				_ports_array = new Raul::Array<PortImpl*>(old_num_ports + 1, *_patch->external_ports(), NULL);
			else
				_ports_array = new Raul::Array<PortImpl*>(old_num_ports + 1, NULL);

			_ports_array->at(old_num_ports) = _patch_port;
			_engine.engine_store()->add(_patch_port);

			if (!_patch->parent()) {
				_engine_port = _engine.driver()->create_port(
						dynamic_cast<DuplexPort*>(_patch_port));
			}

			assert(_ports_array->size() == _patch->num_ports());

		} else {
			_status = CREATION_FAILED;
		}
	}
	Event::pre_process();
}

void
CreatePort::execute(ProcessContext& context)
{
	Event::execute(context);

	if (_patch_port) {
		_engine.maid()->push(_patch->external_ports());
		_patch->external_ports(_ports_array);
	}

	if (_engine_port) {
		_engine.driver()->add_port(_engine_port);
	}
}

void
CreatePort::post_process()
{
	respond(_status);
	if (!_status) {
		_engine.broadcaster()->send_object(_patch_port, true);
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen

