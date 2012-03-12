/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include "raul/Array.hpp"
#include "raul/Atom.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"
#include "ingen/shared/LV2URIMap.hpp"
#include "ClientBroadcaster.hpp"
#include "ControlBindings.hpp"
#include "CreatePort.hpp"
#include "Driver.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "PatchImpl.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {
namespace Events {

CreatePort::CreatePort(Engine&                     engine,
                       ClientInterface*            client,
                       int32_t                     id,
                       SampleCount                 timestamp,
                       const Raul::Path&           path,
                       bool                        is_output,
                       const Resource::Properties& properties)
	: Event(engine, client, id, timestamp)
	, _path(path)
	, _data_type(PortType::UNKNOWN)
	, _patch(NULL)
	, _patch_port(NULL)
	, _ports_array(NULL)
	, _driver_port(NULL)
	, _properties(properties)
	, _is_output(is_output)
{
	const Ingen::Shared::URIs& uris = *_engine.world()->uris().get();

	typedef Resource::Properties::const_iterator Iterator;
	typedef std::pair<Iterator, Iterator>        Range;
	const Range types = properties.equal_range(uris.rdf_type);
	for (Iterator i = types.first; i != types.second; ++i) {
		const Raul::Atom& type = i->second;
		if (type.type() != Atom::URI) {
			warn << "Non-URI port type " << type << endl;
			continue;
		}

		if (type == uris.lv2_AudioPort) {
			_data_type = PortType::AUDIO;
		} else if (type == uris.lv2_ControlPort) {
			_data_type = PortType::CONTROL;
		} else if (type == uris.cv_CVPort) {
			_data_type = PortType::CV;
		} else if (type == uris.ev_EventPort) {
			_data_type = PortType::EVENTS;
		} else if (type == uris.atom_ValuePort) {
			_data_type = PortType::VALUE;
		} else if (type == uris.atom_MessagePort) {
			_data_type = PortType::MESSAGE;
		}
	}

	if (_data_type == PortType::UNKNOWN) {
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

	const Ingen::Shared::URIs& uris = *_engine.world()->uris().get();

	if (_patch != NULL) {
		assert(_patch->path() == _path.parent());

		size_t buffer_size = _engine.buffer_factory()->default_buffer_size(_data_type);

		const uint32_t old_num_ports = (_patch->external_ports())
			? _patch->external_ports()->size()
			: 0;

		Resource::Properties::const_iterator index_i = _properties.find(uris.lv2_index);
		if (index_i == _properties.end()) {
			index_i = _properties.insert(make_pair(uris.lv2_index, (int)old_num_ports));
		} else if (index_i->second.type() != Atom::INT
				|| index_i->second.get_int32() != static_cast<int32_t>(old_num_ports)) {
			Event::pre_process();
			_status = BAD_INDEX;
			return;
		}

		Resource::Properties::const_iterator poly_i = _properties.find(uris.ingen_polyphonic);
		bool polyphonic = (poly_i != _properties.end() && poly_i->second.type() == Atom::BOOL
				&& poly_i->second.get_bool());

		_patch_port = _patch->create_port(*_engine.buffer_factory(), _path.symbol(), _data_type, buffer_size, _is_output, polyphonic);

		_patch_port->properties().insert(_properties.begin(), _properties.end());

		assert(index_i->second == Atom((int)_patch_port->index()));

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

			if (!_patch->parent())
				_driver_port = _engine.driver()->create_port(
						dynamic_cast<DuplexPort*>(_patch_port));

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

	if (_driver_port) {
		_engine.driver()->add_port(_driver_port);
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

} // namespace Server
} // namespace Ingen
} // namespace Events

