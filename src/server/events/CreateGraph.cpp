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

#include "Broadcaster.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "events/CreateGraph.hpp"

namespace Ingen {
namespace Server {
namespace Events {

CreateGraph::CreateGraph(Engine&                     engine,
                         SPtr<Interface>             client,
                         int32_t                     id,
                         SampleCount                 timestamp,
                         const Raul::Path&           path,
                         const Resource::Properties& properties)
	: Event(engine, client, id, timestamp)
	, _path(path)
	, _properties(properties)
	, _graph(NULL)
	, _parent(NULL)
	, _compiled_graph(NULL)
{}

bool
CreateGraph::pre_process()
{
	if (_path.is_root() || _engine.store()->get(_path)) {
		return Event::pre_process_done(Status::EXISTS, _path);
	}

	_parent = dynamic_cast<GraphImpl*>(_engine.store()->get(_path.parent()));
	if (!_parent) {
		return Event::pre_process_done(Status::PARENT_NOT_FOUND, _path.parent());
	}

	const Ingen::URIs& uris = _engine.world()->uris();

	typedef Resource::Properties::const_iterator iterator;

	uint32_t ext_poly = 1;
	uint32_t int_poly = 1;
	iterator p        = _properties.find(uris.ingen_polyphony);
	if (p != _properties.end() && p->second.type() == uris.forge.Int) {
		int_poly = p->second.get<int32_t>();
	}

	if (int_poly < 1 || int_poly > 128) {
		return Event::pre_process_done(Status::INVALID_POLY, _path);
	}

	if (int_poly == _parent->internal_poly()) {
		ext_poly = int_poly;
	}

	const Raul::Symbol symbol((_path.is_root()) ? "root" : _path.symbol());

	// Get graph prototype
	iterator t = _properties.find(uris.lv2_prototype);
	if (t == _properties.end()) {
		t = _properties.find(uris.lv2_prototype);
	}

	if (t != _properties.end() &&
	    Raul::URI::is_valid(t->second.ptr<char>()) &&
	    Node::uri_is_path(Raul::URI(t->second.ptr<char>()))) {
		// Create a duplicate of an existing graph
		const Raul::URI prototype(t->second.ptr<char>());
		GraphImpl*      ancestor = dynamic_cast<GraphImpl*>(
			_engine.store()->get(Node::uri_to_path(prototype)));
		if (!ancestor) {
			return Event::pre_process_done(Status::PROTOTYPE_NOT_FOUND, prototype);
		} else if (!(_graph = dynamic_cast<GraphImpl*>(
			      ancestor->duplicate(_engine, symbol, _parent)))) {
			return Event::pre_process_done(Status::CREATION_FAILED, _path);
		}
	} else {
		// Create a new graph
		_graph = new GraphImpl(_engine, symbol, ext_poly, _parent,
		                       _engine.driver()->sample_rate(), int_poly);
		_graph->add_property(uris.rdf_type, uris.ingen_Graph);
		_graph->add_property(uris.rdf_type,
		                     Resource::Property(uris.ingen_Block,
		                                        Resource::Graph::EXTERNAL));
	}

	_graph->set_properties(_properties);

	_parent->add_block(*_graph);
	if (_parent->enabled()) {
		_graph->enable();
		_compiled_graph = _parent->compile();
	}

	_graph->activate(*_engine.buffer_factory());

	// Insert into store and build update to send to clients
	_engine.store()->add(_graph);
	_update.put_graph(_graph);
	for (BlockImpl& block : _graph->blocks()) {
		_engine.store()->add(&block);
	}

	return Event::pre_process_done(Status::SUCCESS);
}

void
CreateGraph::execute(ProcessContext& context)
{
	if (_graph) {
		_parent->set_compiled_graph(_compiled_graph);
	}
}

void
CreateGraph::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS) {
		_update.send(_engine.broadcaster());
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
