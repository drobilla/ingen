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

#include "ingen/Store.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "raul/Array.hpp"
#include "raul/Atom.hpp"
#include "raul/Path.hpp"

#include "Broadcaster.hpp"
#include "BufferFactory.hpp"
#include "CreatePort.hpp"
#include "Driver.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {
namespace Events {

CreatePort::CreatePort(Engine&                     engine,
                       SPtr<Interface>             client,
                       int32_t                     id,
                       SampleCount                 timestamp,
                       const Raul::Path&           path,
                       bool                        is_output,
                       const Resource::Properties& properties)
	: Event(engine, client, id, timestamp)
	, _path(path)
	, _port_type(PortType::UNKNOWN)
	, _buf_type(0)
	, _graph(NULL)
	, _graph_port(NULL)
	, _ports_array(NULL)
	, _old_ports_array(NULL)
	, _engine_port(NULL)
	, _properties(properties)
	, _is_output(is_output)
{
	const Ingen::URIs& uris = _engine.world()->uris();

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
			_buf_type = _engine.world()->uri_map().map_uri(i->second.ptr<char>());
		}
	}
}

bool
CreatePort::pre_process()
{
	if (_port_type == PortType::UNKNOWN) {
		return Event::pre_process_done(Status::UNKNOWN_TYPE, _path);
	}

	if (_path.is_root()) {
		return Event::pre_process_done(Status::BAD_URI, _path);
	}

	if (_engine.store()->get(_path)) {
		return Event::pre_process_done(_status, _path);
	}

	Node* parent = _engine.store()->get(_path.parent());
	if (!parent) {
		return Event::pre_process_done(Status::PARENT_NOT_FOUND,
		                               _path.parent());
	}

	if (!(_graph = dynamic_cast<GraphImpl*>(parent))) {
		return Event::pre_process_done(Status::INVALID_PARENT_PATH,
		                               _path.parent());
	}

	const URIs&          uris           = _engine.world()->uris();
	const BufferFactory& buffer_factory = *_engine.buffer_factory();

	const uint32_t buf_size    = buffer_factory.default_size(_buf_type);
	const int32_t  old_n_ports = _graph->num_ports_non_rt();

	typedef Resource::Properties::const_iterator PropIter;

	PropIter index_i = _properties.find(uris.lv2_index);
	if (index_i == _properties.end()) {
		// No index given, append
		index_i = _properties.insert(
			std::make_pair(uris.lv2_index,
			               _engine.world()->forge().make(old_n_ports)));
	} else if (index_i->second.type() != uris.forge.Int ||
	           index_i->second.get<int32_t>() != old_n_ports) {
		return Event::pre_process_done(Status::BAD_INDEX, _path);
	}

	const PropIter poly_i = _properties.find(uris.ingen_polyphonic);
	const bool polyphonic = (poly_i != _properties.end() &&
	                         poly_i->second.type() == uris.forge.Bool &&
	                         poly_i->second.get<int32_t>());

	if (!(_graph_port = _graph->create_port(
		      *_engine.buffer_factory(), Raul::Symbol(_path.symbol()),
		      _port_type, _buf_type, buf_size, _is_output, polyphonic))) {
		return Event::pre_process_done(Status::CREATION_FAILED, _path);
	}

	_graph_port->properties().insert(_properties.begin(), _properties.end());

	_engine.store()->add(_graph_port);
	if (_is_output) {
		_graph->add_output(*_graph_port);
	} else {
		_graph->add_input(*_graph_port);
	}

	if (!_graph->parent()) {
		_engine_port = _engine.driver()->create_port(
			dynamic_cast<DuplexPort*>(_graph_port));
	}

	_ports_array = new Raul::Array<PortImpl*>(old_n_ports + 1, NULL);
	_update      = _graph_port->properties();

	assert(_graph_port->index() == (uint32_t)index_i->second.get<int32_t>());
	assert(_graph->num_ports_non_rt() == (uint32_t)old_n_ports + 1);
	assert(_graph_port->index() == (uint32_t)old_n_ports);
	assert(_ports_array->size() == _graph->num_ports_non_rt());
	assert(_graph_port->index() < _ports_array->size());
	return Event::pre_process_done(Status::SUCCESS);
}

void
CreatePort::execute(ProcessContext& context)
{
	if (_status == Status::SUCCESS) {
		_old_ports_array = _graph->external_ports();
		if (_old_ports_array) {
			for (uint32_t i = 0; i < _old_ports_array->size(); ++i) {
				(*_ports_array)[i] = (*_old_ports_array)[i];
			}
		}
		assert(!(*_ports_array)[_graph_port->index()]);
		(*_ports_array)[_graph_port->index()] = _graph_port;
		_graph->external_ports(_ports_array);

		if (_engine_port) {
			_engine.driver()->add_port(context, _engine_port);
		}
	}
}

void
CreatePort::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS) {
		_engine.broadcaster()->put(Node::path_to_uri(_path), _update);
	}

	delete _old_ports_array;
}

} // namespace Events
} // namespace Server
} // namespace Ingen

