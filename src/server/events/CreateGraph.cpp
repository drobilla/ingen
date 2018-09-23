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

#include "ingen/Forge.hpp"
#include "ingen/Store.hpp"
#include "ingen/URIs.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"

#include "Broadcaster.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "PreProcessContext.hpp"
#include "events/CreateGraph.hpp"
#include "events/CreatePort.hpp"

namespace ingen {
namespace server {
namespace events {

CreateGraph::CreateGraph(Engine&           engine,
                         SPtr<Interface>   client,
                         int32_t           id,
                         SampleCount       timestamp,
                         const Raul::Path& path,
                         const Properties& properties)
	: Event(engine, client, id, timestamp)
	, _path(path)
	, _properties(properties)
	, _graph(nullptr)
	, _parent(nullptr)
{}

CreateGraph::~CreateGraph()
{
	for (Event* ev : _child_events) {
		delete ev;
	}
}

void
CreateGraph::build_child_events()
{
	const ingen::URIs& uris = _engine.world()->uris();

	// Properties common to both ports
	Properties control_properties;
	control_properties.put(uris.atom_bufferType, uris.atom_Sequence);
	control_properties.put(uris.atom_supports, uris.patch_Message);
	control_properties.put(uris.lv2_designation, uris.lv2_control);
	control_properties.put(uris.lv2_portProperty, uris.lv2_connectionOptional);
	control_properties.put(uris.rdf_type, uris.atom_AtomPort);
	control_properties.put(uris.rsz_minimumSize, uris.forge.make(4096));

	// Add control port (message receive)
	Properties in_properties(control_properties);
	in_properties.put(uris.lv2_index, uris.forge.make(0));
	in_properties.put(uris.lv2_name, uris.forge.alloc("Control"));
	in_properties.put(uris.rdf_type, uris.lv2_InputPort);
	in_properties.put(uris.ingen_canvasX, uris.forge.make(32.0f),
	                  Resource::Graph::EXTERNAL);
	in_properties.put(uris.ingen_canvasY, uris.forge.make(32.0f),
	                  Resource::Graph::EXTERNAL);

	_child_events.push_back(
		new events::CreatePort(
			_engine, _request_client, -1, _time,
			_path.child(Raul::Symbol("control")),
			in_properties));

	// Add notify port (message respond)
	Properties out_properties(control_properties);
	out_properties.put(uris.lv2_index, uris.forge.make(1));
	out_properties.put(uris.lv2_name, uris.forge.alloc("Notify"));
	out_properties.put(uris.rdf_type, uris.lv2_OutputPort);
	out_properties.put(uris.ingen_canvasX, uris.forge.make(128.0f),
	                   Resource::Graph::EXTERNAL);
	out_properties.put(uris.ingen_canvasY, uris.forge.make(32.0f),
	                   Resource::Graph::EXTERNAL);

	_child_events.push_back(
		new events::CreatePort(_engine, _request_client, -1, _time,
		                       _path.child(Raul::Symbol("notify")),
		                       out_properties));
}

bool
CreateGraph::pre_process(PreProcessContext& ctx)
{
	if (_engine.store()->get(_path)) {
		return Event::pre_process_done(Status::EXISTS, _path);
	}

	if (!_path.is_root()) {
		const Raul::Path up(_path.parent());
		if (!(_parent = dynamic_cast<GraphImpl*>(_engine.store()->get(up)))) {
			return Event::pre_process_done(Status::PARENT_NOT_FOUND, up);
		}
	}

	const ingen::URIs& uris = _engine.world()->uris();

	typedef Properties::const_iterator iterator;

	uint32_t ext_poly = 1;
	uint32_t int_poly = 1;
	iterator p        = _properties.find(uris.ingen_polyphony);
	if (p != _properties.end() && p->second.type() == uris.forge.Int) {
		int_poly = p->second.get<int32_t>();
	}

	if (int_poly < 1 || int_poly > 128) {
		return Event::pre_process_done(Status::INVALID_POLY, _path);
	}

	if (!_parent || int_poly == _parent->internal_poly()) {
		ext_poly = int_poly;
	}

	const Raul::Symbol symbol(_path.is_root() ? "graph" : _path.symbol());

	// Get graph prototype
	iterator t = _properties.find(uris.lv2_prototype);
	if (t == _properties.end()) {
		t = _properties.find(uris.lv2_prototype);
	}

	if (t != _properties.end() &&
	    uris.forge.is_uri(t->second) &&
	    URI::is_valid(uris.forge.str(t->second, false)) &&
	    uri_is_path(URI(uris.forge.str(t->second, false)))) {
		// Create a duplicate of an existing graph
		const URI  prototype(uris.forge.str(t->second, false));
		GraphImpl* ancestor = dynamic_cast<GraphImpl*>(
			_engine.store()->get(uri_to_path(prototype)));
		if (!ancestor) {
			return Event::pre_process_done(Status::PROTOTYPE_NOT_FOUND, prototype);
		} else if (!(_graph = dynamic_cast<GraphImpl*>(
			      ancestor->duplicate(_engine, symbol, _parent)))) {
			return Event::pre_process_done(Status::CREATION_FAILED, _path);
		}
	} else {
		// Create a new graph
		_graph = new GraphImpl(_engine, symbol, ext_poly, _parent,
		                       _engine.sample_rate(), int_poly);
		_graph->add_property(uris.rdf_type, uris.ingen_Graph.urid);
		_graph->add_property(uris.rdf_type,
		                     Property(uris.ingen_Block,
		                              Resource::Graph::EXTERNAL));
	}

	_graph->set_properties(_properties);

	if (_parent) {
		// Add graph to parent
		_parent->add_block(*_graph);
		if (_parent->enabled()) {
			_graph->enable();
		}
		_compiled_graph = ctx.maybe_compile(*_engine.maid(), *_parent);
	}

	_graph->activate(*_engine.buffer_factory());

	// Insert into store and build update to send to clients
	_engine.store()->add(_graph);
	_update.put_graph(_graph);
	for (BlockImpl& block : _graph->blocks()) {
		_engine.store()->add(&block);
	}

	// Build and pre-process child events to create standard ports
	build_child_events();
	for (Event* ev : _child_events) {
		ev->pre_process(ctx);
	}

	return Event::pre_process_done(Status::SUCCESS);
}

void
CreateGraph::execute(RunContext& context)
{
	if (_graph) {
		if (_parent) {
			if (_compiled_graph) {
				_parent->set_compiled_graph(std::move(_compiled_graph));
			}
		} else {
			_engine.set_root_graph(_graph);
			_graph->enable();
		}

		for (Event* ev : _child_events) {
			ev->execute(context);
		}
	}
}

void
CreateGraph::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS) {
		_update.send(*_engine.broadcaster());
	}

	if (_graph) {
		for (Event* ev : _child_events) {
			ev->post_process();
		}
	}
}

void
CreateGraph::undo(Interface& target)
{
	target.del(_graph->uri());
}

} // namespace events
} // namespace server
} // namespace ingen
