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

#include "CreatePort.hpp"

#include "Broadcaster.hpp"
#include "BufferFactory.hpp"
#include "Driver.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "PortImpl.hpp"

#include "ingen/Atom.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Store.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "raul/Array.hpp"
#include "raul/Path.hpp"

#include <cassert>
#include <utility>

namespace ingen {
namespace server {
namespace events {

CreatePort::CreatePort(Engine&                engine,
                       const SPtr<Interface>& client,
                       int32_t                id,
                       SampleCount            timestamp,
                       const Raul::Path&      path,
                       const Properties&      properties)
	: Event(engine, client, id, timestamp)
	, _path(path)
	, _port_type(PortType::UNKNOWN)
	, _buf_type(0)
	, _graph(nullptr)
	, _graph_port(nullptr)
	, _engine_port(nullptr)
	, _properties(properties)
{
	const ingen::URIs& uris = _engine.world().uris();

	using Iterator = Properties::const_iterator;
	using Range    = std::pair<Iterator, Iterator>;

	const Range types = properties.equal_range(uris.rdf_type);
	for (Iterator i = types.first; i != types.second; ++i) {
		const Atom& type = i->second;
		if (type == uris.lv2_AudioPort) {
			_port_type = PortType::AUDIO;
		} else if (type == uris.lv2_ControlPort) {
			_port_type = PortType::CONTROL;
		} else if (type == uris.lv2_CVPort) {
			_port_type = PortType::CV;
		} else if (type == uris.atom_AtomPort) {
			_port_type = PortType::ATOM;
		} else if (type == uris.lv2_InputPort) {
			_flow = Flow::INPUT;
		} else if (type == uris.lv2_OutputPort) {
			_flow = Flow::OUTPUT;
		}
	}

	const Range buffer_types = properties.equal_range(uris.atom_bufferType);
	for (Iterator i = buffer_types.first; i != buffer_types.second; ++i) {
		if (uris.forge.is_uri(i->second)) {
			_buf_type = _engine.world().uri_map().map_uri(
				uris.forge.str(i->second, false));
		}
	}
}

bool
CreatePort::pre_process(PreProcessContext&)
{
	if (_port_type == PortType::UNKNOWN) {
		return Event::pre_process_done(Status::UNKNOWN_TYPE, _path);
	} else if (!_flow) {
		return Event::pre_process_done(Status::UNKNOWN_TYPE, _path);
	} else if (_path.is_root()) {
		return Event::pre_process_done(Status::BAD_URI, _path);
	} else if (_engine.store()->get(_path)) {
		return Event::pre_process_done(Status::EXISTS, _path);
	}

	const Raul::Path parent_path = _path.parent();
	Node* const      parent      = _engine.store()->get(parent_path);
	if (!parent) {
		return Event::pre_process_done(Status::PARENT_NOT_FOUND, parent_path);
	} else if (!(_graph = dynamic_cast<GraphImpl*>(parent))) {
		return Event::pre_process_done(Status::INVALID_PARENT, parent_path);
	} else if (!_graph->parent() && _engine.activated() &&
	           !_engine.driver()->dynamic_ports()) {
		return Event::pre_process_done(Status::CREATION_FAILED, _path);
	}

	const URIs&    uris        = _engine.world().uris();
	BufferFactory& bufs        = *_engine.buffer_factory();
	const uint32_t buf_size    = bufs.default_size(_buf_type);
	const int32_t  old_n_ports = _graph->num_ports_non_rt();

	using PropIter = Properties::const_iterator;

	PropIter index_i = _properties.find(uris.lv2_index);
	int32_t  index   = 0;
	if (index_i != _properties.end()) {
		// Ensure given index is sane and not taken
		if (index_i->second.type() != uris.forge.Int) {
			return Event::pre_process_done(Status::BAD_REQUEST);
		}

		index = index_i->second.get<int32_t>();
		if (_graph->has_port_with_index(index)) {
			return Event::pre_process_done(Status::BAD_INDEX);
		}
	} else {
		// No index given, append
		index   = old_n_ports;
		index_i = _properties.emplace(uris.lv2_index,
		                              _engine.world().forge().make(index));
	}

	const PropIter poly_i = _properties.find(uris.ingen_polyphonic);
	const bool polyphonic = (poly_i != _properties.end() &&
	                         poly_i->second.type() == uris.forge.Bool &&
	                         poly_i->second.get<int32_t>());

	// Create 0 value if the port requires one
	Atom value;
	if (_port_type == PortType::CONTROL || _port_type == PortType::CV) {
		value = bufs.forge().make(0.0f);
	}

	// Create port
	_graph_port = new DuplexPort(bufs, _graph, Raul::Symbol(_path.symbol()),
	                             index,
	                             polyphonic,
	                             _port_type, _buf_type, buf_size,
	                             value, _flow == Flow::OUTPUT);
	assert((_flow == Flow::OUTPUT && _graph_port->is_output()) ||
	       (_flow == Flow::INPUT && _graph_port->is_input()));
	_graph_port->properties().insert(_properties.begin(), _properties.end());

	_engine.store()->add(_graph_port);
	if (_flow == Flow::OUTPUT) {
		_graph->add_output(*_graph_port);
	} else {
		_graph->add_input(*_graph_port);
	}

	if (!_graph->parent()) {
		_engine_port = _engine.driver()->create_port(_graph_port);
	}

	_ports_array = bufs.maid().make_managed<GraphImpl::Ports>(
		old_n_ports + 1, nullptr);

	_update = _graph_port->properties();

	assert(_graph_port->index() == (uint32_t)index_i->second.get<int32_t>());
	assert(_graph->num_ports_non_rt() == (uint32_t)old_n_ports + 1);
	assert(_ports_array->size() == _graph->num_ports_non_rt());
	assert(_graph_port->index() < _ports_array->size());
	return Event::pre_process_done(Status::SUCCESS);
}

void
CreatePort::execute(RunContext& context)
{
	if (_status == Status::SUCCESS) {
		const MPtr<GraphImpl::Ports>& old_ports = _graph->external_ports();
		if (old_ports) {
			for (uint32_t i = 0; i < old_ports->size(); ++i) {
				const auto* const old_port = (*old_ports)[i];
				assert(old_port->index() < _ports_array->size());
				(*_ports_array)[old_port->index()] = (*old_ports)[i];
			}
		}
		assert(!(*_ports_array)[_graph_port->index()]);
		(*_ports_array)[_graph_port->index()] = _graph_port;
		_graph->set_external_ports(std::move(_ports_array));

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
		_engine.broadcaster()->put(path_to_uri(_path), _update);
	}
}

void
CreatePort::undo(Interface& target)
{
	target.del(_graph_port->uri());
}

} // namespace events
} // namespace server
} // namespace ingen
